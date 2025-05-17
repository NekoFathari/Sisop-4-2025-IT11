#include "shm_common.h"
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void cleanup() {
    key_t system_key = get_system_key();
    int shmid = shmget(system_key, sizeof(struct SystemData), 0666);
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
    }
}

void handle_signal(int sig) {
    cleanup();
    exit(0);
}

void generate_dungeon(struct SystemData *system_data) {
    pthread_mutex_lock(&mutex);
    
    if (system_data->num_dungeons >= MAX_DUNGEONS) {
        printf("Dungeon limit reached!\n");
        pthread_mutex_unlock(&mutex);
        return;
    }

    char *dungeon_names[] = {
        "Double Dungeon", "Demon Castle", "Pyramid Dungeon", 
        "Red Gate Dungeon", "Hunters Guild Dungeon", "Busan A-Rank Dungeon",
        "Insects Dungeon", "Goblins Dungeon", "D-Rank Dungeon",
        "Gwanak Mountain Dungeon", "Hapjeong Subway Station Dungeon"
    };
    int num_names = sizeof(dungeon_names) / sizeof(dungeon_names[0]);

    struct Dungeon new_dungeon;
    strcpy(new_dungeon.name, dungeon_names[rand() % num_names]);
    new_dungeon.min_level = (rand() % 5) + 1;
    new_dungeon.exp = (rand() % 151) + 150;     // 150-300
    new_dungeon.atk = (rand() % 51) + 100;      // 100-150
    new_dungeon.hp = (rand() % 51) + 50;        // 50-100
    new_dungeon.def = (rand() % 26) + 25;       // 25-50
    new_dungeon.shm_key = rand();

    system_data->dungeons[system_data->num_dungeons++] = new_dungeon;
    
    printf("Dungeon generated!\n");
    printf("Name: %s\n", new_dungeon.name);
    printf("Minimum Level: %d\n", new_dungeon.min_level);
    
    pthread_mutex_unlock(&mutex);
}

void show_hunter_info(struct SystemData *system_data) {
    printf("--- HUNTER INFO ---\n");
    for (int i = 0; i < system_data->num_hunters; i++) {
        printf("Name: %s\n", system_data->hunters[i].username);
        printf("Level: %d EXP: %d ATK: %d HP: %d DEF: %d %s\n", 
               system_data->hunters[i].level, 
               system_data->hunters[i].exp,
               system_data->hunters[i].atk,
               system_data->hunters[i].hp,
               system_data->hunters[i].def,
               system_data->hunters[i].banned ? "[BANNED]" : "");
        printf("-------------------\n");
    }
}

void show_dungeon_info(struct SystemData *system_data) {
    printf("--- DUNGEON INFO ---\n");
    for (int i = 0; i < system_data->num_dungeons; i++) {
        printf("[Dungeon %d]\n", i+1);
        printf("Name: %s\n", system_data->dungeons[i].name);
        printf("Minimum Level: %d\n", system_data->dungeons[i].min_level);
        printf("EXP Reward: %d\n", system_data->dungeons[i].exp);
        printf("ATK: %d\n", system_data->dungeons[i].atk);
        printf("HP: %d\n", system_data->dungeons[i].hp);
        printf("DEF: %d\n", system_data->dungeons[i].def);
        printf("Key: %d\n", system_data->dungeons[i].shm_key);
        printf("-------------------\n");
    }
}

void ban_hunter(struct SystemData *system_data) {
    printf("Enter hunter username to ban: ");
    char username[50];
    scanf("%s", username);
    
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < system_data->num_hunters; i++) {
        if (strcmp(system_data->hunters[i].username, username) == 0) {
            system_data->hunters[i].banned = !system_data->hunters[i].banned;
            printf("%s has been %s\n", username, 
                   system_data->hunters[i].banned ? "banned" : "unbanned");
            pthread_mutex_unlock(&mutex);
            return;
        }
    }
    printf("Hunter not found!\n");
    pthread_mutex_unlock(&mutex);
}

void reset_hunter(struct SystemData *system_data) {
    printf("Enter hunter username to reset: ");
    char username[50];
    scanf("%s", username);
    
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < system_data->num_hunters; i++) {
        if (strcmp(system_data->hunters[i].username, username) == 0) {
            system_data->hunters[i].level = 1;
            system_data->hunters[i].exp = 0;
            system_data->hunters[i].atk = 10;
            system_data->hunters[i].hp = 100;
            system_data->hunters[i].def = 5;
            printf("%s has been reset to initial stats\n", username);
            pthread_mutex_unlock(&mutex);
            return;
        }
    }
    printf("Hunter not found!\n");
    pthread_mutex_unlock(&mutex);
}

void *notification_thread(void *arg) {
    struct SystemData *system_data = (struct SystemData *)arg;
    
    while (1) {
        pthread_mutex_lock(&mutex);
        if (system_data->num_dungeons > 0) {
            system_data->current_notification_index = 
                (system_data->current_notification_index + 1) % system_data->num_dungeons;
        }
        pthread_mutex_unlock(&mutex);
        sleep(3);
    }
    
    return NULL;
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    atexit(cleanup);
    
    srand(time(NULL));
    
    key_t system_key = get_system_key();
    int shmid = shmget(system_key, sizeof(struct SystemData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    
    struct SystemData *system_data = (struct SystemData *)shmat(shmid, NULL, 0);
    if (system_data == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    
    system_data->num_hunters = 0;
    system_data->num_dungeons = 0;
    system_data->current_notification_index = 0;
    
    pthread_t notif_thread;
    pthread_create(&notif_thread, NULL, notification_thread, system_data);
    
    while (1) {
        printf("\n--- SYSTEM MENU ---\n");
        printf("1. Hunter Info\n");
        printf("2. Dungeon Info\n");
        printf("3. Generate Dungeon\n");
        printf("4. Ban Hunter\n");
        printf("5. Reset Hunter\n");
        printf("6. Exit\n");
        printf("Choice: ");
        
        int choice;
        scanf("%d", &choice);
        
        switch (choice) {
            case 1:
                show_hunter_info(system_data);
                break;
            case 2:
                show_dungeon_info(system_data);
                break;
            case 3:
                generate_dungeon(system_data);
                break;
            case 4:
                ban_hunter(system_data);
                break;
            case 5:
                reset_hunter(system_data);
                break;
            case 6:
                cleanup();
                exit(0);
            default:
                printf("Invalid choice!\n");
        }
    }
    
    return 0;
}