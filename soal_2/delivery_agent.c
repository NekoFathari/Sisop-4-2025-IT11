
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/wait.h>

#define SHM_KEY 1234 
#define MAX_ORDERS 100
#define CSV_FILENAME "delivery_order.csv"
#define DOWNLOAD_LINK "https://drive.usercontent.google.com/u/0/uc?id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9&export=download"
#define LOG_FILENAME "delivery.log"

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

int file_exists(const char* filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

void get_timestamp(char* buffer, size_t len) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, len, "[%d/%m/%Y %H:%M:%S]", t);
}

int main() {
    if (!file_exists(CSV_FILENAME)) {
        printf("File %s tidak ditemukan. Mengunduh...\n", CSV_FILENAME);

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            char* args[] = {"wget", "-O", CSV_FILENAME, DOWNLOAD_LINK, NULL};
            execvp("wget", args);
            // Jika execvp gagal
            perror("execvp gagal");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                fprintf(stderr, "Gagal mengunduh file CSV.\n");
                return 1;
            }
        } else {
            // Fork gagal
            perror("fork gagal");
            return 1;
        }

        printf("Berhasil mengunduh %s\n", CSV_FILENAME);
    }

    FILE* file = fopen(CSV_FILENAME, "r");
    if (!file) {
        perror("Gagal membuka file CSV");
        return 1;
    }

   
    FILE* log_file = fopen(LOG_FILENAME, "w");
    if (!log_file) {
        perror("Gagal membuat delivery.log");
        fclose(file);
        return 1;
    }
    fclose(log_file);

    int shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget gagal");
        fclose(file);
        return 1;
    }

    SharedMemory* shm = (SharedMemory*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) {
        perror("shmat gagal");
        fclose(file);
        return 1;
    }

    shm->order_count = 0;

    
    srand(time(NULL));

    char line[256];
    int line_number = 0;

    log_file = fopen(LOG_FILENAME, "a");
    if (!log_file) {
        perror("Gagal membuka file log");
        fclose(file);
        return 1;
    }

    while (fgets(line, sizeof(line), file)) {
        line_number++;
        if (line_number == 1) continue; // skip header

        if (shm->order_count >= MAX_ORDERS) {
            fprintf(stderr, "Melebihi batas maksimum order.\n");
            break;
        }

        char* token = strtok(line, ",");
        if (!token) continue;
        strncpy(shm->orders[shm->order_count].name, token, 99);

        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(shm->orders[shm->order_count].address, token, 99);

        token = strtok(NULL, ",\n");
        if (!token) continue;
        strncpy(shm->orders[shm->order_count].type, token, 9);

        strcpy(shm->orders[shm->order_count].status, "Pending");

        // Tulis log
        char timestamp[32];
        get_timestamp(timestamp, sizeof(timestamp));

        if (strcmp(shm->orders[shm->order_count].type, "Express") == 0) {
            char agent = 'A' + (rand() % 3); // A, B, atau C
            strcpy(shm->orders[shm->order_count].status, "Delivered");
            fprintf(log_file, "%s [AGENT %c] Express package delivered to %s in %s\n",
                    timestamp,
                    agent,
                    shm->orders[shm->order_count].name,
                    shm->orders[shm->order_count].address);
        }

        shm->order_count++;
    }

    printf("Berhasil memuat %d order ke shared memory dan mencatat ke log.\n", shm->order_count);

    fclose(file);
    fclose(log_file);
    shmdt(shm);

    return 0;
}
