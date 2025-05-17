#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>

#define SHM_KEY 1234
#define MAX_ORDERS 100

typedef struct {
    char name[100];
    char address[100];
    char type[10];      
    char status[20];   
} Order;

typedef struct {
    Order orders[MAX_ORDERS];
    int order_count;
} SharedMemory;


void log_delivery(const char* type, const char* name, const char* address) {
    FILE *log_file = fopen("delivery.log", "a");
    if (!log_file) {
        perror("Error opening log file");
        return;
    }

    char *user = getenv("USER");
    if (!user) user = "UNKNOWN";

    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%d/%m/%Y %H:%M:%S", time_info);

    fprintf(log_file, "[%s] [AGENT %s] %s package delivered to %s in %s\n",
            time_str, user, type, name, address);

    fclose(log_file);
}


void check_status(SharedMemory *shm, const char* name) {
    for (int i = 0; i < shm->order_count; i++) {
        if (strcmp(shm->orders[i].name, name) == 0) {
            if (strcmp(shm->orders[i].status, "Delivered") == 0) {
                FILE *log_file = fopen("delivery.log", "r");
                if (!log_file) {
                    perror("Gagal membuka delivery.log");
                    return;
                }

                char line[256];
                char agent_name[100] = "UNKNOWN";
                while (fgets(line, sizeof(line), log_file)) {
                    if (strstr(line, name)) {
                        char *start = strstr(line, "[AGENT ");
                        if (start) {
                            start += 7; // lewati "[AGENT "
                            char *end = strchr(start, ']');
                            if (end) {
                                *end = '\0';
                                strncpy(agent_name, start, sizeof(agent_name));
                                break;
                            }
                        }
                    }
                }
                fclose(log_file);

                printf("Status for %s: Delivered by Agent %s\n", name, agent_name);
            } else {
                printf("Status for %s: Pending\n", name);
            }
            return;
        }
    }
    printf("Order not found.\n");
}


void list_orders(SharedMemory *shm) {
    printf("Listing all orders:\n");
    for (int i = 0; i < shm->order_count; i++) {
        printf("Name: %s, Status: %s\n", shm->orders[i].name, shm->orders[i].status);
    }
}

int main(int argc, char* argv[]) {
    int shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    SharedMemory *shm = (SharedMemory*)shmat(shmid, NULL, 0);
    if (shm == (SharedMemory*)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: ./dispatcher [-deliver Nama | -status Nama | -list]\n");
        shmdt(shm);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
        const char* recipient = argv[2];
        int found = 0;

        for (int i = 0; i < shm->order_count; i++) {
            if (strcmp(shm->orders[i].name, recipient) == 0 &&
                strcmp(shm->orders[i].type, "Reguler") == 0 &&
                strcmp(shm->orders[i].status, "Pending") == 0) {

                strcpy(shm->orders[i].status, "Delivered");
                log_delivery("Reguler", shm->orders[i].name, shm->orders[i].address);
                printf("Reguler order for '%s' marked as Delivered.\n", recipient);
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("Reguler order for '%s' not found or already delivered.\n", recipient);
        }

    } else if (strcmp(argv[1], "-status") == 0 && argc == 3) {
        check_status(shm, argv[2]);
    } else if (strcmp(argv[1], "-list") == 0) {
        list_orders(shm);
    } else {
        fprintf(stderr, "Invalid command.\n");
    }

    shmdt(shm);
    return 0;
}

