#include "shm_common.h"
#include <stdbool.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct HunterData {
    char username[50];
    int level;
    int exp;
    int atk;
    int hp;
    int def;
    int banned;
    int notification_enabled;
};

// Fungsi untuk sinkronisasi stat hunter ke sistem
void sync_hunter_stats(struct SystemData *system_data, struct HunterData *hunter_data) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < system_data->num_hunters; i++) {
        if (strcmp(system_data->hunters[i].username, hunter_data->username) == 0) {
            system_data->hunters[i].level = hunter_data->level;
            system_data->hunters[i].exp = hunter_data->exp;
            system_data->hunters[i].atk = hunter_data->atk;
            system_data->hunters[i].hp = hunter_data->hp;
            system_data->hunters[i].def = hunter_data->def;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
}

void show_available_dungeons(struct SystemData *system_data, int hunter_level) {
    printf("\n--- AVAILABLE DUNGEONS ---\n");
    int count = 0;
    for (int i = 0; i < system_data->num_dungeons; i++) {
        if (system_data->dungeons[i].min_level <= hunter_level) {
            printf("%d. %s (Level %d+)\n", ++count, 
                   system_data->dungeons[i].name, 
                   system_data->dungeons[i].min_level);
        }
    }
    if (count == 0) {
        printf("No dungeons available for your level.\n");
    }
    printf("\nPress enter to continue...");
    getchar(); getchar();
}

void raid_dungeon(struct SystemData *system_data, struct HunterData *hunter) {
    show_available_dungeons(system_data, hunter->level);
    
    printf("Choose Dungeon: ");
    int choice;
    scanf("%d", &choice);
    
    int dungeon_index = -1;
    int count = 0;
    for (int i = 0; i < system_data->num_dungeons; i++) {
        if (system_data->dungeons[i].min_level <= hunter->level) {
            if (++count == choice) {
                dungeon_index = i;
                break;
            }
        }
    }
    
    if (dungeon_index == -1) {
        printf("Invalid dungeon choice!\n");
        return;
    }
    
    pthread_mutex_lock(&mutex);
    
    // Apply rewards
    hunter->exp += system_data->dungeons[dungeon_index].exp;
    hunter->atk += system_data->dungeons[dungeon_index].atk;
    hunter->hp += system_data->dungeons[dungeon_index].hp;
    hunter->def += system_data->dungeons[dungeon_index].def;
    
    // Check level up
    if (hunter->exp >= 500) {
        hunter->level++;
        hunter->exp = 0;
        printf("Level up! You are now level %d\n", hunter->level);
    }
    
    printf("\nRaid success! Rewards:\n");
    printf("ATK: %d\n", system_data->dungeons[dungeon_index].atk);
    printf("HP: %d\n", system_data->dungeons[dungeon_index].hp);
    printf("DEF: %d\n", system_data->dungeons[dungeon_index].def);
    printf("EXP: %d\n", system_data->dungeons[dungeon_index].exp);
    
    // Remove dungeon
    for (int i = dungeon_index; i < system_data->num_dungeons - 1; i++) {
        system_data->dungeons[i] = system_data->dungeons[i+1];
    }
    system_data->num_dungeons--;
    
    pthread_mutex_unlock(&mutex);
    
    // Sinkronkan stat ke sistem
    sync_hunter_stats(system_data, hunter);
    
    printf("\nPress enter to continue...");
    getchar(); getchar();
}

void battle_hunter(struct SystemData *system_data, struct HunterData *hunter) {
    printf("\n--- PVP LIST ---\n");
    for (int i = 0; i < system_data->num_hunters; i++) {
        if (strcmp(system_data->hunters[i].username, hunter->username) != 0) {
            int power = system_data->hunters[i].atk + system_data->hunters[i].hp + system_data->hunters[i].def;
            printf("%s - Total Power: %d\n", system_data->hunters[i].username, power);
        }
    }
    
    printf("Target: ");
    char target[50];
    scanf("%s", target);
    
    int target_index = -1;
    for (int i = 0; i < system_data->num_hunters; i++) {
        if (strcmp(system_data->hunters[i].username, target) == 0) {
            target_index = i;
            break;
        }
    }
    
    if (target_index == -1) {
        printf("Hunter not found!\n");
        return;
    }
    
    pthread_mutex_lock(&mutex);
    
    int your_power = hunter->atk + hunter->hp + hunter->def;
    int opponent_power = system_data->hunters[target_index].atk + 
                         system_data->hunters[target_index].hp + 
                         system_data->hunters[target_index].def;
    
    printf("You chose to battle %s\n", target);
    printf("Your Power: %d\n", your_power);
    printf("Opponent's Power: %d\n", opponent_power);
    
    if (your_power > opponent_power) {
        // You win
        hunter->atk += system_data->hunters[target_index].atk;
        hunter->hp += system_data->hunters[target_index].hp;
        hunter->def += system_data->hunters[target_index].def;
        
        // Remove opponent
        for (int i = target_index; i < system_data->num_hunters - 1; i++) {
            system_data->hunters[i] = system_data->hunters[i+1];
        }
        system_data->num_hunters--;
        
        printf("Battle won! You acquired %s's stats\n", target);
    } else {
        // You lose
        system_data->hunters[target_index].atk += hunter->atk;
        system_data->hunters[target_index].hp += hunter->hp;
        system_data->hunters[target_index].def += hunter->def;
        
        printf("Battle lost! You have been defeated by %s\n", target);
        printf("Your hunter has been removed from the system.\n");
        
        pthread_mutex_unlock(&mutex);
        exit(0);
    }
    
    pthread_mutex_unlock(&mutex);
    
    // Sinkronkan stat ke sistem
    sync_hunter_stats(system_data, hunter);
    
    printf("\nPress enter to continue...");
    getchar(); getchar();
}

void *notification_handler(void *arg) {
    struct HunterData *hunter = (struct HunterData *)arg;
    key_t system_key = get_system_key();
    
    while (hunter->notification_enabled) {
        int shmid = shmget(system_key, sizeof(struct SystemData), 0666);
        if (shmid == -1) {
            sleep(3);
            continue;
        }
        
        struct SystemData *system_data = (struct SystemData *)shmat(shmid, NULL, 0);
        if (system_data == (void *)-1) {
            sleep(3);
            continue;
        }
        
        pthread_mutex_lock(&mutex);
        if (system_data->num_dungeons > 0) {
            int idx = system_data->current_notification_index;
            if (system_data->dungeons[idx].min_level <= hunter->level) {
                printf("\n--- NOTIFICATION ---\n");
                printf("Available Dungeon: %s (Level %d+)\n", 
                       system_data->dungeons[idx].name, 
                       system_data->dungeons[idx].min_level);
                printf("Rewards: EXP:%d ATK:%d HP:%d DEF:%d\n",
                       system_data->dungeons[idx].exp,
                       system_data->dungeons[idx].atk,
                       system_data->dungeons[idx].hp,
                       system_data->dungeons[idx].def);
            }
        }
        pthread_mutex_unlock(&mutex);
        
        shmdt(system_data);
        sleep(3);
    }
    
    return NULL;
}

void notification(struct HunterData *hunter) {
    hunter->notification_enabled = !hunter->notification_enabled;
    printf("Notifications %s\n", hunter->notification_enabled ? "enabled" : "disabled");
    
    if (hunter->notification_enabled) {
        pthread_t notif_thread;
        pthread_create(&notif_thread, NULL, notification_handler, hunter);
    }
}

int main() {
    key_t system_key = get_system_key();
    int system_shmid = shmget(system_key, sizeof(struct SystemData), 0666);
    if (system_shmid == -1) {
        printf("System is not running. Please start the system first.\n");
        exit(1);
    }
    
    struct SystemData *system_data = (struct SystemData *)shmat(system_shmid, NULL, 0);
    if (system_data == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    
    // Hunter menu
    while (1) {
        printf("\n--- HUNTER MENU ---\n");
        printf("1. Register\n");
        printf("2. Login\n");
        printf("3. Exit\n");
        printf("Choice: ");
        
        int choice;
        scanf("%d", &choice);
        
        if (choice == 1) {
            // Register
            printf("Username: ");
            char username[50];
            scanf("%s", username);
            
            pthread_mutex_lock(&mutex);
            
            // Check if username exists
            int exists = 0;
            for (int i = 0; i < system_data->num_hunters; i++) {
                if (strcmp(system_data->hunters[i].username, username) == 0) {
                    exists = 1;
                    break;
                }
            }
            
            if (exists) {
                printf("Username already exists!\n");
                pthread_mutex_unlock(&mutex);
                continue;
            }
            
            // Create hunter shared memory
            key_t hunter_key = ftok("/tmp", username[0]);
            int hunter_shmid = shmget(hunter_key, sizeof(struct HunterData), IPC_CREAT | 0666);
            if (hunter_shmid == -1) {
                perror("shmget");
                pthread_mutex_unlock(&mutex);
                continue;
            }
            
            struct HunterData *hunter_data = (struct HunterData *)shmat(hunter_shmid, NULL, 0);
            if (hunter_data == (void *)-1) {
                perror("shmat");
                pthread_mutex_unlock(&mutex);
                continue;
            }
            
            // Initialize hunter data
            strcpy(hunter_data->username, username);
            hunter_data->level = 1;
            hunter_data->exp = 0;
            hunter_data->atk = 10;
            hunter_data->hp = 100;
            hunter_data->def = 5;
            hunter_data->banned = 0;
            hunter_data->notification_enabled = 0;
            
            // Add to system
            strcpy(system_data->hunters[system_data->num_hunters].username, username);
            system_data->hunters[system_data->num_hunters].level = 1;
            system_data->hunters[system_data->num_hunters].exp = 0;
            system_data->hunters[system_data->num_hunters].atk = 10;
            system_data->hunters[system_data->num_hunters].hp = 100;
            system_data->hunters[system_data->num_hunters].def = 5;
            system_data->hunters[system_data->num_hunters].banned = 0;
            system_data->hunters[system_data->num_hunters].shm_key = hunter_key;
            system_data->num_hunters++;
            
            printf("Registration success!\n");
            
            shmdt(hunter_data);
            pthread_mutex_unlock(&mutex);
            
        } else if (choice == 2) {
            // Login
            printf("Username: ");
            char username[50];
            scanf("%s", username);
            
            key_t hunter_key = -1;
            for (int i = 0; i < system_data->num_hunters; i++) {
                if (strcmp(system_data->hunters[i].username, username) == 0) {
                    if (system_data->hunters[i].banned) {
                        printf("This hunter is banned!\n");
                        break;
                    }
                    hunter_key = system_data->hunters[i].shm_key;
                    break;
                }
            }
            
            if (hunter_key == -1) {
                printf("Hunter not found!\n");
                continue;
            }
            
            int hunter_shmid = shmget(hunter_key, sizeof(struct HunterData), 0666);
            if (hunter_shmid == -1) {
                perror("shmget");
                continue;
            }
            
            struct HunterData *hunter_data = (struct HunterData *)shmat(hunter_shmid, NULL, 0);
            if (hunter_data == (void *)-1) {
                perror("shmat");
                continue;
            }
            
            // Hunter system menu
            while (1) {
                printf("\n--- %s's MENU ---\n", hunter_data->username);
                printf("1. List Dungeon\n");
                printf("2. Raid\n");
                printf("3. Battle\n");
                printf("4. Notification\n");
                printf("5. Exit\n");
                printf("Choice: ");
                
                int hunter_choice;
                scanf("%d", &hunter_choice);
                
                switch (hunter_choice) {
                    case 1:
                        show_available_dungeons(system_data, hunter_data->level);
                        break;
                    case 2:
                        if (hunter_data->banned) {
                            printf("You are banned from raiding!\n");
                        } else {
                            raid_dungeon(system_data, hunter_data);
                        }
                        break;
                    case 3:
                        if (hunter_data->banned) {
                            printf("You are banned from battling!\n");
                        } else {
                            battle_hunter(system_data, hunter_data);
                        }
                        break;
                    case 4:
                        notification(hunter_data);
                        break;
                    case 5:
                        shmdt(hunter_data);
                        goto logout;
                    default:
                        printf("Invalid choice!\n");
                }
            }
            
            logout:
            continue;
            
        } else if (choice == 3) {
            // Exit
            shmdt(system_data);
            exit(0);
        } else {
            printf("Invalid choice!\n");
        }
    }
    
    return 0;
}