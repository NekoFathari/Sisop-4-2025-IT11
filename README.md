# Sisop-3-2025-IT11

### MEMBER
1. Muhammad Ardiansyah Tri Wibowo - 5027241091
2. Erlinda Annisa Zahra Kusuma - 5027241108
3. Fika Arka Nuriyah - 5027241071


### REPORTING 

### SOAL 1
### SOAL 2
RushGo ingin memberikan layanan ekspedisi dengan 2 pilihan ada reguler dan express. lalu membuat delivery management system untuk RushGo.
Sistem ini memiliki 2 bagian utama:
- delivery_agent.c sebagai agent pengantar express
- dispatcher.c sebagai pengiriman reguler dan monitoring oleh user

a. Mengunduh file delivery_order.csv lalu membaca seluruh data dari CSV dan menyimpan ke shared memory.

Mengunduh file

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

Membaca File CSV

      FILE* file = fopen(CSV_FILENAME, "r");
    if (!file) {
        perror("Gagal membuka file CSV");
        return 1;
    }

Menyimpan ke Shared Memory

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


b. Pengiriman bertipe express
RushGo disini memakai 3 agent ada agent A,B dan C. Agent akan secara otomatis mencari order bertipe express yang belum dikirim. Mengambil dan mengirimkannya tanpa user. lalu akan di catat dalam log.

            
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

lalu dicatat dalam log 

            char timestamp[32];
            get_timestamp(timestamp, sizeof(timestamp));


c. Pengiriman bertipe reguler
Disini pengirimannya dilakukan dengan agent <user>. User disini dapat mengirim dengan menggunakan perintah dispatcher.

Pengiriman dengan reguler

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

d. Mengecek Status pesanan

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

e. Melihat daftar semua pesanan
Bisa menjalankan perintah list untuk melihat semua order nama disertai status

            void list_orders(SharedMemory *shm) {
    printf("Listing all orders:\n");
    for (int i = 0; i < shm->order_count; i++) {
        printf("Name: %s, Status: %s\n", shm->orders[i].name, shm->orders[i].status);
          }
      } 
            
            

### SOAL 3

Pada tahap ini terdapat 8 point yang perlu dibuat dalam menyelesaikan permasalahan ini.

a. dungeon.c dan player.c berjalan dengan komunikasi RPC, dan server dapat menjalankan banyak client dalam 1 server.

#### dungeon.C
```
// Library yang perlu digunakan
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
....

int main(){
    ...
    // sesuai yang ada di modul
    pthread_mutex_init(&client_lock, NULL); // Inisialisasi mutex

    if(( servernya = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    alamat.sin_family = AF_INET;
    alamat.sin_addr.s_addr = INADDR_ANY;
    alamat.sin_port = htons(PORT);

    if(bind(servernya, (struct sockaddr *)&alamat, sizeof(alamat)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(servernya, 3) < 0){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    ...

    while(1){
        printf("Menunggu koneksi...\n");
        int* new_socket = (int*)malloc(sizeof(int));
        if ((*new_socket = accept(servernya, (struct sockaddr*)&alamat, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            free(new_socket);
            continue; // lanjut ke iterasi berikutnya
        }
        printf("Koneksi diterima\n");

        // Buat thread baru untuk menangani client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, new_socket) != 0) {
            perror("pthread_create failed");
            close(*new_socket);
            free(new_socket);
            continue; // lanjut ke iterasi berikutnya
        }

        // Detach thread agar sumber daya dibebaskan setelah selesai
        pthread_detach(thread_id);
    }
}

```

#### dungeon.c (dalam handle banyak client)
```
void handle_client(...){
...
while (1) {
    int pilihan_dari_client;
    int bytes_read = read(socketnya, &pilihan_dari_client, sizeof(pilihan_dari_client));
    if (bytes_read < 0) {
        perror("read failed");
        break; // Keluar dari perulangan jika terjadi error
    }
    if (bytes_read == 0) {
        printf("Client dengan port %d terputus.\n", client_port);
        break; // Keluar dari perulangan jika client menutup koneksi
    }
...
}
```

### player.c 
```
int main(){
    ....

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        // Jika koneksi gagal, kita bisa menampilkan pesan error
        loading(20);
        fprintf(stderr, "\033[1;31m");
        perror("Connection failed");
        fprintf(stderr, "\033[0m");

        exit(EXIT_FAILURE);
    }
    ....
}
```

b. Menampilkan main menu, dan listener hubungan dungeon.c dan player.c

#### player.C
```
void logo(){
printf("\033[H\033[J");
printf("██████╗░██╗░░░██╗███╗░░██╗░██████╗░███████╗░█████╗░███╗░░██╗\n");
printf("██╔══██╗██║░░░██║████╗░██║██╔════╝░██╔════╝██╔══██╗████╗░██║\n");
printf("██║░░██║██║░░░██║██╔██╗██║██║░░██╗░█████╗░░██║░░██║██╔██╗██║\n");
printf("██║░░██║██║░░░██║██║╚████║██║░░╚██╗██╔══╝░░██║░░██║██║╚████║\n");
printf("██████╔╝╚██████╔╝██║░╚███║╚██████╔╝███████╗╚█████╔╝██║░╚███║\n");
printf("╚═════╝░░╚═════╝░╚═╝░░╚══╝░╚═════╝░╚══════╝░╚════╝░╚═╝░░╚══╝\n\n");
}

void menu(){
    printf("\033[H\033[J");
    logo();
    printf("1. Show Player Stats\n");
    printf("2. Inventory\n");
    printf("3. Shop (Buy Weapons)\n");
    printf("4. Battle\n");
    printf("5. Exit Game\n\n");
    printf(RED "   Choose an option: " RESET);

    int pilihanku;
    scanf("%d", &pilihanku);
    switch (pilihanku) {
        ....
    }

}    
```

c. Menampilkan status check 
#### dungeon.c (proses background)
```
....
if (send(socketnya, &player[nomor_player], sizeof(player[nomor_player]), 0) <= 0) {
    perror("send failed (client stats)");
    break; // Keluar dari perulangan jika terjadi error
}
printf("Stats dikirim ke client dengan port %d\n", client_port);
....
```

#### player.c
```
....
// Terima data stats dari server
    PlayerStats stats;
    if (recv(sock, &stats, sizeof(stats), 0) <= 0) {
        perror("Receive failed (stats)");
        close(sock);
        exit(EXIT_FAILURE);
    }
....
```

d. Weapon Shop
#### shop.H
```
....
typedef struct {
    ....
} Senjata;

Senjata senjata[5] = {
    ....
};

void beli_senjata(....) {
....
```

#### dungeon.C
```
....
beli_senjata(...);
....
```

#### player.c
```
....
if (recv(sock, senjata, sizeof(senjata), 0) <= 0) {
    .....
    }
// Terima data uang dari server
int uang;
if (recv(sock, &uang, sizeof(uang), 0) <= 0) {
    ....
    }
....

int pilihan_senjata;
scanf("%d", &pilihan_senjata);

if (pilihan_senjata < 0 || pilihan_senjata > 5) {
    printf("Pilihan tidak valid. Kembali ke menu.\n");
    return;
}
if (send(sock, &pilihan_senjata, sizeof(pilihan_senjata), 0) < 0) {
....
}
....
```

e. Inventory
#### dungeon.c 
```
....
if (send(socketnya, &inv[nomor_player], sizeof(inv[nomor_player]), 0) <= 0) {
    perror("send failed (client inventory)");
    break; // Keluar dari perulangan jika terjadi error
}
....
bool ada_item = false;
// print isinya
for (int i = 0; i < 20; i++) {
    if (inv[nomor_player][i].id != 0) {
        ada_item = true;
        ....
    }
}

// Kita dapatkan respon dari client mau pilih senjata mana buat dipasang
// Terima pilihan senjata dari client
int pilihan_senjata;
if (recv(socketnya, &pilihan_senjata, sizeof(pilihan_senjata), 0) <= 0) {
    .....
}
if(!ada_item){
    .....
} else {
    ....
}

if (pilihan_senjata < 0 || pilihan_senjata >= sizeof(inv[nomor_player]) / sizeof(Inventory)) {
    .....
}

if (pilihan_senjata == 0) {
    ......
}

// Cek apakah client memiliki barang tersebut di Inventory
for (int i = 0; i < 20; i++) {
    if (inv[nomor_player][i].id == pilihan_senjata) {
        // reset statsnya kepada base stats
        .....

        // Pasangkan ke statsnya
        ......

        char msg[100] = {0};
        snprintf(msg, sizeof(msg) ,"Senjata %s berhasil di pasang\n", inv[nomor_player][i].nama);
        if (send(socketnya, msg, sizeof(msg), 0) <= 0) {
            ......
        }
        printf("Client memasang senjata dengan id %d\n", inv[nomor_player][i].id);
        break; // Keluar dari perulangan jika terjadi error
    }
}
....
```

#### player.c  
```
....
// Terima data inventory dari server
Inventory inventory[20];
if (recv(sock, &inventory, sizeof(inventory), 0) <= 0) {
    .....
}

.....

int slot;
scanf("%d", &slot);
if (slot < 0 || slot > 20) {
    .....
}
if (send(sock, &slot, sizeof(slot), 0) < 0) {
    ......
}
// Terima response
char response[100];
if (recv(sock, response, sizeof(response), 0) <= 0) {
    ....
}
....
```

f. Enemy Battle dan g. Other logic
#### dungeon.c 
```
....

//randomize musuh dulu
int random_musuh = rand() % 10;
if (random_musuh >= 3) {
    random_musuh = rand() % 3; // Lebih sering memilih 0, 1, atau 2
}
musuhnya musuh_terpilih = musuh[random_musuh];
......    
// udah dapet musuhnya healthnya kita random dari min_darah ke max_darah
int health_musuh = (rand() % (musuh_terpilih.max_darah - musuh_terpilih.min_darah + 1)) + musuh_terpilih.min_darah;

// Karena udah dapet musuh, dan healthnya, Cus kita kirim ke client
if (send(socketnya, &musuh_terpilih, sizeof(musuh_terpilih), 0) <= 0) {
    ......
}
if (send(socketnya, &health_musuh, sizeof(health_musuh), 0) <= 0) {
    ......
}
.....
while (1) {                        
    // Terima respon dari client, antara 1 untuk menyerang atau 2 untuk kabur
    int pilihan_battle;
    if (recv(socketnya, &pilihan_battle, sizeof(pilihan_battle), 0) <= 0) {
        ......
    }
    // exit code
    if (pilihan_battle == 2) {
        printf("Client dengan port %d memilih untuk kabur\n", client_port);
        // set health client ke 100 lagi, well reset lah
        player[nomor_player].darah = darahnya; // Reset health player
        break; // Keluar dari perulangan jika client memilih kabur
    }
    // aksi code
    if (pilihan_battle == 1) {
        // Dapatkan damage dari player, crit, crit_damage
        .......

        if (crit_serangnya != 0 && crit_damage_pas_serang != 0) {
            int crit = rand() % 100;
            if (crit < crit_serangnya) {
                sakitnya_diserang = sakitnya_diserang + (sakitnya_diserang * crit_damage_pas_serang);
                health_musuh -= sakitnya_diserang;
            } else {
                health_musuh -= sakitnya_diserang;
            }
        } else {
            int random_damage_userlah = (rand() % ((int)(sakitnya_diserang * 1.2) - (int)(sakitnya_diserang * 0.7) + 1)) + (int)(sakitnya_diserang * 0.7);
            if (random_damage_userlah > sakitnya_diserang) {
                health_musuh -= random_damage_userlah;
            } else {
                health_musuh -= random_damage_userlah;
            }
        }
        // kirim health musuh ke client dan kalau mati musuh random masuk, dan healthnya di random

        if (health_musuh <= 0) {
            health_musuh = 0; // Set health musuh ke 0                                
            if (send(socketnya, &health_musuh, sizeof(health_musuh), 0) <= 0) {
                .....
            }

            // Kirim health musuh ke client
            int random_musuh = rand() % 10;
            if (random_musuh >= 3) {
                random_musuh = rand() % 3; // Lebih sering memilih 0, 1, atau 2
            }
            musuhnya musuh_terpilih = musuh[random_musuh];    
            // udah dapet musuhnya healthnya kita random dari min_darah ke max_darah
            health_musuh = (rand() % (musuh_terpilih.max_darah - musuh_terpilih.min_darah + 1)) + musuh_terpilih.min_darah;

            // Karena udah dapet musuh, dan healthnya, Cus kita kirim ke client

            if (send(socketnya, &musuh_terpilih, sizeof(musuh_terpilih), 0) <= 0) {
                .......
            }
            if (send(socketnya, &health_musuh, sizeof(health_musuh), 0) <= 0) {
                ......
            }

            // Tambah uang player
            int uang_dapat = (rand() % 41) + 20; // Uang dapat antara 20 hingga 60
            player[nomor_player].uang += uang_dapat;
            if (send(socketnya, &uang_dapat, sizeof(uang_dapat), 0) <= 0) {
                perror("send failed (uang dapat)");
                break; // Keluar dari perulangan jika terjadi error
            }

            // Kirim health player ke client
            .....

            // Musuh menyerang balik
            int damage_musuh = rand() % (musuh_terpilih.damage + 1); // Random damage antara 0 hingga damage maksimum musuh
            player[nomor_player].darah -= damage_musuh;

            // Kirim health player ke client
            if (send(socketnya, &player[nomor_player].darah, sizeof(player[nomor_player].darah), 0) <= 0) {
                .....
            }

            if (player[nomor_player].darah <= 0) {
                printf("Client dengan port %d telah kalah dalam pertarungan\n", client_port);
                player[nomor_player].darah = darahnya; // Reset health player
                player[nomor_player].berapa_mati++; // Tambah berapa kali mati
                printf("Health player direset ke %d dan health musuh direset ke %d\n", darahnya, health_musuh);
                break; // Keluar dari perulangan jika player kalah
            }
        
        } 
    }

....
```
#### player.C
```
....
// Terima data musuh dari server
    if (recv(sock, &musuhku, sizeof(musuhku), 0) <= 0) {
        .....
    }

    // Terima health musuh dari server
    int health_musuh_max;
    if (recv(sock, &health_musuh_max, sizeof(health_musuh_max), 0) <= 0) {
        .....
    }

    int health_musuh = health_musuh_max;
    int pilihan_battle;

    do {
        ......
        printf("%s\n", musuhku.nama);
        if (health_musuh_max >= health_musuh) {
            health_barwak(health_musuh, health_musuh_max);
        }

        printf("Pilih aksi (1 untuk menyerang, 2 untuk kabur): ");
        scanf("%d", &pilihan_battle);

        if (pilihan_battle < 1 || pilihan_battle > 2) {
            .......
        }

        if (send(sock, &pilihan_battle, sizeof(pilihan_battle), 0) < 0) {
            .......
        }

        if (pilihan_battle == 2) {
           .......
        }

        // Terima health musuh yang diperbarui dari server
        if (recv(sock, &health_musuh, sizeof(health_musuh), 0) <= 0) {
            .....
        }

        if (health_musuh <= 0) {
            printf("Musuh telah dikalahkan!\n");
            sleep(2);

            // Terima musuh yang baru dari server
            if (recv(sock, &musuhku, sizeof(musuhku), 0) <= 0) {
                .....
            }
            // Terima health musuh baru dari server
            if (recv(sock, &health_musuh_max, sizeof(health_musuh_max), 0) <= 0) {
               .......
            }
            health_musuh = health_musuh_max;
            // Dapet notif uang
            int uang_dapat;
            if (recv(sock, &uang_dapat, sizeof(uang_dapat), 0) <= 0) {
                .......
            }
            printf("Anda mendapatkan %d uang!\n", uang_dapat);
            sleep(2);
            printf("Musuh baru muncul!: %s\n", musuhku.nama);
            sleep(2);
        }

        // Terima darah pemain yang diperbarui dari server
        int darahku;
        if (recv(sock, &darahku, sizeof(darahku), 0) <= 0) {
            .......
        }

        if(darahku <= 0) {
            .....
            break;
        }
        printf("Darah Anda: (%d/%d)\n", darahku, 100);
        sleep(2);

    } while (pilihan_battle != 2);
}
....
```

h. Error handling
#### player.C
```
....
int menu(){
    .....
    default:
    printf("\033[1;31mInvalid choice. Please try again.\033[0m\n");
    sleep(2);
    menu();
}
....
```

### SOAL 4
system.c
Header & Inisialisasi

```#include "shm_common.h"
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
```

Mengimpor definisi struktur dari `shm_common.h`.
Inisialisasi mutex global untuk menjaga data shared memory tetap aman saat diakses banyak thread.
``` cleanup()

void cleanup() {
    key_t system_key = get_system_key();
    int shmid = shmget(system_key, sizeof(struct SystemData), 0666);
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
    }
}
```
- Menghapus shared memory `SystemData` dari sistem saat program keluar.
- Digunakan saat `exit()` atau saat menerima sinyal `SIGINT/SIGTERM`.
 handle_signal(int sig)
```
void handle_signal(int sig) {
    cleanup();
    exit(0);
}
```

 Fungsi handler untuk menangani sinyal sistem seperti `Ctrl+C`.
 Memanggil `cleanup()` sebelum keluar.
 
      generate_dungeon(struct SystemData *system_data)
Menghasilkan dungeon baru dengan parameter random (level, atk, exp, dll). Disimpan di array `system_data->dungeons`.
Dungeon memiliki nama yang diambil dari daftar dungeon preset.
 Menggunakan mutex untuk menjaga konsistensi saat menambahkan dungeon.

       show_hunter_info(struct SystemData *system_data)
Menampilkan seluruh data hunter:
- Username
- Level, EXP, ATK, HP, DEF
- Status banned
Berguna bagi admin untuk memonitor pemain.

         show_dungeon_info(struct SystemData *system_data)
Menampilkan seluruh dungeon yang telah dibuat:
- Nama, level minimal, reward EXP, stats dungeon, dll.

        ban_hunter(struct SystemData *system_data)
Menerima username hunter. Jika ditemukan, toggle `banned` (ban/unban). Mutex digunakan untuk menjaga konsistensi update.
       
       reset_hunter(struct SystemData *system_data)
Mengatur ulang hunter ke status awal:
- Level 1, EXP 0, ATK 10, HP 100, DEF 5
Digunakan jika hunter bermasalah atau ingin memulai ulang.

        notification_thread(void *arg)
Thread yang berjalan terus-menerus. Tiap 3 detik, menggilir index dungeon aktif (`current_notification_index`) untuk sistem notifikasi.

      main()
Fungsi utama program admin.
Melakukan:

- Setup sinyal dan `atexit()`
- Alokasi shared memory `SystemData`
- Inisialisasi nilai default
- Menjalankan thread notifikasi
- Menampilkan menu utama admin secara loop:
  1. Tampilkan hunter
  2. Tampilkan dungeon
  3. Generate dungeon baru
  4. Ban/unban hunter
  5. Reset hunter
  6. Keluar program

### hunter.c
Penjelasan Lengkap Kode Hunter Client System
Header & Struktur
```
#include "shm_common.h"
#include <stdbool.h>
```
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

- Header `shm_common.h` digunakan untuk akses struktur data bersama.
- Mutex digunakan untuk menjaga sinkronisasi saat beberapa thread/proses mengakses shared memory.

        struct HunterData
Struktur ini menyimpan data karakter hunter seperti username, level, EXP, stat, status banned, dan pengaturan notifikasi.

       sync_hunter_stats()
Melakukan sinkronisasi stat hunter yang aktif ke sistem global `SystemData` agar data tetap konsisten setelah perubahan (raid, battle, dll).

      show_available_dungeons()
Menampilkan dungeon yang bisa diakses berdasarkan level hunter.
Menunggu input enter agar user bisa membaca info sebelum lanjut.

       raid_dungeon()
- Menampilkan dungeon yang bisa diserang.
- Memberikan reward (exp, atk, hp, def).
- Mengecek apakah level naik setelah exp cukup.
- Menghapus dungeon dari sistem setelah diserang.
- Update sistem global melalui `sync_hunter_stats()`.

        battle_hunter()
- Menampilkan daftar hunter lain untuk PVP.
- Jika menang, stat musuh diakuisisi dan musuh dihapus.
- Jika kalah, stat kita diberikan ke musuh dan kita keluar dari sistem.
- Menjaga sinkronisasi melalui mutex dan `sync_hunter_stats()`.

         notification_handler()
- Thread yang menampilkan notifikasi dungeon aktif jika level hunter memenuhi syarat.
- Berjalan setiap 3 detik saat `notification_enabled` aktif.

        notification()
Mengaktifkan atau menonaktifkan notifikasi dungeon dan membuat thread `notification_handler()` jika aktif.
 
       main()
- Menghubungkan hunter ke shared memory sistem.
- Menyediakan menu utama:
  1. Register
  2. Login
  3. Exit
- Saat login, hunter dapat mengakses:
  1. List Dungeon
  2. Raid
  3. Battle
  4. Notification
  5. Exit
- Data hunter akan dihapus dari shared memory saat logout.

  ### Dokumentasi
  ![image](https://github.com/user-attachments/assets/7bef7a74-3ec2-428d-a17c-c8d82d2c6b73)
  
  ![image](https://github.com/user-attachments/assets/fb4007fe-0cbc-41bb-b8c1-39dafdf3031e)
  
  ![image](https://github.com/user-attachments/assets/1327e0ef-69ef-4b02-99d9-c302d512f7a5)
  
  ![image](https://github.com/user-attachments/assets/2638a5a7-d0ee-44af-8778-c441233439bc)

  ![image](https://github.com/user-attachments/assets/2c5d82c5-f616-407a-a958-f7e70eff2225)

  ![image](https://github.com/user-attachments/assets/ee96db79-712a-49bb-a206-61d3e87b629a)

  ![image](https://github.com/user-attachments/assets/9d7c67d5-cad1-4734-b8ff-fa4e95685586)

  ![image](https://github.com/user-attachments/assets/d7abb9c0-b573-4d07-be90-88be8c2f5fc2)

  ![image](https://github.com/user-attachments/assets/b11b4ba7-05db-4e56-bfcc-19f5c95e7705)









