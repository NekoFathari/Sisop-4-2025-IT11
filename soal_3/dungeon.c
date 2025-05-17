#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#define PORT 8001
#define BUFFER_SIZE 1024
#define client_maxnya 10

// Base stats
#define darahnya 100
#define base_sakitnya 5
#define base_uangnya 5000
#define senjatanya "Tangan Kosong"
#define terbunuh 0 // jangan diubah, kalau di tambah berarti udah pernah mati wkwkwkw

typedef struct {
    char nama[20];
    int harga;
    int damage;
    int crit;
    int crit_damage;
} Senjatanya;

typedef struct {
    int port;
    bool aktif;
} Client;

typedef struct {
    int uang;
    int damage;
    int darah;
    int berapa_mati;
    int crit;
    int crit_damage;
    char senjata[50];
} stat_player;

typedef struct {
    int id; // [Alokasikan id 1-10 untuk Senjata, 11-infinity untuk inventory lainnya seperti sampah wkwkw]
    char nama[50];
    int harga_jual;
    int banyaknya;
} Inventory;

typedef struct{
    char nama[20];
    int min_darah;
    int max_darah;
    int damage;
} musuhnya;

musuhnya musuh[5] = {
    {"Goblin", 20, 40, 5},
    {"Orc", 30, 50, 10},
    {"Troll", 20, 40, 10},
    {"Dragon", 60, 120, 25},
    {"Basilisk", 50, 70, 20}
};

Inventory inv[client_maxnya][20] = {0}; // Array untuk menyimpan inventory
Client clients[client_maxnya]; // Array untuk menyimpan data client
stat_player player[client_maxnya]; // Array untuk menyimpan data player

int hitung_clientnya = 0;
pthread_mutex_t client_lock;

// Library untuk shop
#include "shop.h"

void* handle_client(void* arg) {
    int socketnya = *(int*)arg;
    free(arg); // Bebaskan memori yang dialokasikan untuk argumen
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Dapatkan informasi client (IP dan port)
    getpeername(socketnya, (struct sockaddr*)&client_addr, &addr_len);
    int client_port = ntohs(client_addr.sin_port);

    // Simpan data client ke array
    pthread_mutex_lock(&client_lock);
    if (hitung_clientnya < client_maxnya) {
        clients[hitung_clientnya].port = client_port;
        clients[hitung_clientnya].aktif = true;

        // Inisialisasi stat_player untuk client ini
        memset(&player[hitung_clientnya], 0, sizeof(stat_player)); // Pastikan semua field diinisialisasi ke 0
        player[hitung_clientnya].uang = base_uangnya;
        player[hitung_clientnya].damage = base_sakitnya;
        player[hitung_clientnya].darah = darahnya;
        strcpy(player[hitung_clientnya].senjata, senjatanya);
        player[hitung_clientnya].berapa_mati = terbunuh;
        player[hitung_clientnya].crit = 0;
        player[hitung_clientnya].crit_damage = 0;
        hitung_clientnya++;

        printf("Client dengan port %d ditambahkan. Total client: %d\n", client_port, hitung_clientnya);
    } else {
        printf("Server penuh. Client dengan port %d ditolak.\n", client_port);
        pthread_mutex_unlock(&client_lock);
        close(socketnya);
        return NULL;
    }
    pthread_mutex_unlock(&client_lock);

    // Perulangan untuk terus membaca data dari client
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

        if (pilihan_dari_client < 0 || pilihan_dari_client > 4) {
            printf("Pilihan tidak valid dari client dengan port %d: %d\n", client_port, pilihan_dari_client);
            continue; // Lanjut ke iterasi berikutnya jika pilihan tidak valid
        }
        switch(pilihan_dari_client) {
            case 1: {
                printf("Client dengan port %d memilih pilihan 1 untuk menampilkan stats\n", client_port);
                // Cari index player berdasarkan port
                int nomor_player = -1;
                pthread_mutex_lock(&client_lock);
                for (int i = 0; i < hitung_clientnya; i++) {
                    if (clients[i].port == client_port) {
                        nomor_player = i;
                        break;
                    }
                }
                pthread_mutex_unlock(&client_lock);

                if (nomor_player != -1) {
                    // Kirim data stats ke client
                    if (send(socketnya, &player[nomor_player], sizeof(player[nomor_player]), 0) <= 0) {
                        perror("send failed (client stats)");
                        break; // Keluar dari perulangan jika terjadi error
                    }
                    printf("Stats dikirim ke client dengan port %d\n", client_port);
                    // print isinya
                    printf("Uang: %d, Damage: %d, Darah: %d, ID Senjata: %d, Berapa Mati: %d, Crit: %d, Crit Damage: %d\n",
                           player[nomor_player].uang,
                           player[nomor_player].damage,
                           player[nomor_player].darah,
                           player[nomor_player].senjata,
                           player[nomor_player].berapa_mati,
                           player[nomor_player].crit,
                           player[nomor_player].crit_damage);
                           
                } else {
                    printf("Player dengan port %d tidak ditemukan.\n", client_port);
                }
                break;
            }
            case 2: {
                printf("Client dengan port %d memilih pilihan 2 untuk menampilkan inventory\n", client_port);
                int nomor_player = -1;
                pthread_mutex_lock(&client_lock);
                for (int i = 0; i < hitung_clientnya; i++) {
                    if (clients[i].port == client_port) {
                        nomor_player = i;
                        break;
                    }
                }
                pthread_mutex_unlock(&client_lock);

                if (nomor_player != -1) {
                    // Kirim data inventory ke client
                    if (send(socketnya, &inv[nomor_player], sizeof(inv[nomor_player]), 0) <= 0) {
                        perror("send failed (client inventory)");
                        break; // Keluar dari perulangan jika terjadi error
                    }
                    printf("Inventory dikirim ke client dengan port %d\n", client_port);

                    bool ada_item = false;
                    // print isinya
                    for (int i = 0; i < 20; i++) {
                        if (inv[nomor_player][i].id != 0) {
                            ada_item = true;
                            printf("ID: %d, Nama: %s, Harga Jual: %d, Banyaknya: %d\n",
                                   inv[nomor_player][i].id,
                                   inv[nomor_player][i].nama,
                                   inv[nomor_player][i].harga_jual,
                                   inv[nomor_player][i].banyaknya);
                        }
                    }

                    // Kita dapatkan respon dari client mau pilih senjata mana buat dipasang
                    // Terima pilihan senjata dari client
                    int pilihan_senjata;
                    if (recv(socketnya, &pilihan_senjata, sizeof(pilihan_senjata), 0) <= 0) {
                        send(socketnya, "recv failed", sizeof("recv failed"), 0);
                        perror("recv failed (pilihan senjata)");
                        break; // Keluar dari perulangan jika terjadi error
                    }

                    if(!ada_item){
                        printf("Client tidak memiliki item di inventory\n");
                        send(socketnya, "Anda tidak memiliki item di inventory", sizeof("Anda tidak memiliki item di inventory"), 0);
                        break; // Keluar dari perulangan jika terjadi error
                    } else {
                        printf("Client memiliki item di inventory\n");
                    }

                    if (pilihan_senjata < 0 || pilihan_senjata >= sizeof(inv[nomor_player]) / sizeof(Inventory)) {
                        printf("Client memasukan pilihan senjata tidak valid\n");
                        send(socketnya, "Pilihan tidak valid", sizeof("Pilihan tidak valid"), 0);
                        break; // Keluar dari perulangan jika terjadi error
                    }


                    if (pilihan_senjata == 0) {
                        printf("Client tidak memilih untuk tidak memasang senjata\n");
                        send(socketnya, "Anda tidak memasang senjata", sizeof("Anda tidak memasang senjata"), 0);
                        
                        // reset statsnya kepada base stats
                        strcpy(player[nomor_player].senjata, senjatanya);
                        player[nomor_player].damage = base_sakitnya;
                        player[nomor_player].crit = 0;
                        player[nomor_player].crit_damage = 0;

                        break; // Keluar dari perulangan jika terjadi error
                    }

                    // Cek apakah client memiliki barang tersebut di Inventory
                    for (int i = 0; i < 20; i++) {
                        if (inv[nomor_player][i].id == pilihan_senjata) {
                            // reset statsnya kepada base stats
                            strcpy(player[nomor_player].senjata, senjatanya);
                            player[nomor_player].damage = base_sakitnya;
                            player[nomor_player].crit = 0;
                            player[nomor_player].crit_damage = 0;

                            // Pasangkan ke statsnya
                            strcpy(player[nomor_player].senjata, inv[nomor_player][i].nama);
                            player[nomor_player].damage += senjata[inv[nomor_player][i].id - 1].damage; // Misalnya senjata id 1-5
                            player[nomor_player].crit += senjata[inv[nomor_player][i].id - 1].crit;
                            player[nomor_player].crit_damage += senjata[inv[nomor_player][i].id - 1].crit_damage;

                            char msg[100] = {0};
                            snprintf(msg, sizeof(msg) ,"Senjata %s berhasil di pasang\n", inv[nomor_player][i].nama);
                            if (send(socketnya, msg, sizeof(msg), 0) <= 0) {
                                perror("send failed (pasang senjata)");
                                break; // Keluar dari perulangan jika terjadi error
                            }
                            printf("Client memasang senjata dengan id %d\n", inv[nomor_player][i].id);
                            break; // Keluar dari perulangan jika terjadi error
                        }
                    }

                    // Pasangkan ke statsnya

                } else {
                    printf("Player dengan port %d tidak ditemukan.\n", client_port);
                }
                break;
            }
            case 3: {
                printf("Client dengan port %d memilih pilihan 3 untuk berbelanja senjata\n", client_port);
                // Panggil fungsi dari shop.h untuk menangani pembelian senjata
                int nomor_player = -1;
                pthread_mutex_lock(&client_lock);
                for (int i = 0; i < hitung_clientnya; i++) {
                    if (clients[i].port == client_port) {
                        nomor_player = i;
                        break;
                    }
                }
                pthread_mutex_unlock(&client_lock);
                
                if (nomor_player != -1) {
                    beli_senjata(socketnya, &player[nomor_player], &inv[nomor_player][0]);
                } else {
                    printf("Player dengan port %d tidak ditemukan.\n", client_port);
                }
                break;
            }
            case 4: {
                printf("Client dengan port %d memilih pilihan 4 untuk bertarung\n", client_port);
                int nomor_player = -1;
                pthread_mutex_lock(&client_lock);
                for (int i = 0; i < hitung_clientnya; i++) {
                    if (clients[i].port == client_port) {
                        nomor_player = i;
                        break;
                    }
                }
                pthread_mutex_unlock(&client_lock);
                if (nomor_player != -1) {
                    
                    //randomize musuh dulu
                    int random_musuh = rand() % 10;
                    if (random_musuh >= 3) {
                        random_musuh = rand() % 3; // Lebih sering memilih 0, 1, atau 2
                    }
                    musuhnya musuh_terpilih = musuh[random_musuh];
                    printf("Musuh yang terpilih: %s (Darah: %d-%d, Damage: %d)\n",
                           musuh_terpilih.nama,
                           musuh_terpilih.min_darah,
                           musuh_terpilih.max_darah,
                           musuh_terpilih.damage);
                     
                    // udah dapet musuhnya healthnya kita random dari min_darah ke max_darah
                    int health_musuh = (rand() % (musuh_terpilih.max_darah - musuh_terpilih.min_darah + 1)) + musuh_terpilih.min_darah;

                    // Karena udah dapet musuh, dan healthnya, Cus kita kirim ke client

                    if (send(socketnya, &musuh_terpilih, sizeof(musuh_terpilih), 0) <= 0) {
                        perror("send failed (musuh)");
                        break; // Keluar dari perulangan jika terjadi error
                    }
                    if (send(socketnya, &health_musuh, sizeof(health_musuh), 0) <= 0) {
                        perror("send failed (health musuh)");
                        break; // Keluar dari perulangan jika terjadi error
                    }
                    printf("Musuh dan healthnya dikirim ke client dengan port %d\n", client_port);
                    
                    while (1) {                        
                        // Terima respon dari client, antara 1 untuk menyerang atau 2 untuk kabur

                        int pilihan_battle;
                        if (recv(socketnya, &pilihan_battle, sizeof(pilihan_battle), 0) <= 0) {
                            perror("recv failed (pilihan battle)");
                            break; // Keluar dari perulangan jika terjadi error
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
                            printf("Client dengan port %d memilih untuk menyerang\n", client_port);
                            // Dapatkan damage dari player, crit, crit_damage
                            int sakitnya_diserang = player[nomor_player].damage;
                            int crit_serangnya = player[nomor_player].crit;
                            int crit_damage_pas_serang = player[nomor_player].crit_damage;

                            if (crit_serangnya != 0 && crit_damage_pas_serang != 0) {
                                int crit = rand() % 100;
                                if (crit < crit_serangnya) {
                                    sakitnya_diserang = sakitnya_diserang + (sakitnya_diserang * crit_damage_pas_serang);
                                    printf("Client dengan port %d melakukan serangan kritis! Damage: %d\n", client_port, sakitnya_diserang);
                                    health_musuh -= sakitnya_diserang;
                                } else {
                                    printf("Client dengan port %d melakukan serangan biasa! Damage: %d\n", client_port, sakitnya_diserang);
                                    health_musuh -= sakitnya_diserang;
                                }
                            } else {
                                int random_damage_userlah = (rand() % ((int)(sakitnya_diserang * 1.2) - (int)(sakitnya_diserang * 0.7) + 1)) + (int)(sakitnya_diserang * 0.7);
                                if (random_damage_userlah > sakitnya_diserang) {
                                    printf("Client dengan port %d melakukan serangan luar biasa! Damage: %d\n", client_port, random_damage_userlah);
                                    health_musuh -= random_damage_userlah;
                                } else {
                                    printf("Client dengan port %d melakukan serangan biasa! Damage: %d\n", client_port, random_damage_userlah);
                                    health_musuh -= random_damage_userlah;
                                }
                            }
                            // kirim health musuh ke client dan kalau mati musuh random masuk, dan healthnya di random

                            if (health_musuh <= 0) {
                                health_musuh = 0; // Set health musuh ke 0                                
                                if (send(socketnya, &health_musuh, sizeof(health_musuh), 0) <= 0) {
                                    perror("send failed (health musuh)");
                                    break; // Keluar dari perulangan jika terjadi error
                                }

                                // Kirim health musuh ke client
                                int random_musuh = rand() % 10;
                                if (random_musuh >= 3) {
                                    random_musuh = rand() % 3; // Lebih sering memilih 0, 1, atau 2
                                }
                                musuhnya musuh_terpilih = musuh[random_musuh];
                                printf("Musuh yang terpilih: %s (Darah: %d-%d, Damage: %d)\n",
                                       musuh_terpilih.nama,
                                       musuh_terpilih.min_darah,
                                       musuh_terpilih.max_darah,
                                       musuh_terpilih.damage);
                                 
                                // udah dapet musuhnya healthnya kita random dari min_darah ke max_darah
                                health_musuh = (rand() % (musuh_terpilih.max_darah - musuh_terpilih.min_darah + 1)) + musuh_terpilih.min_darah;
            
                                // Karena udah dapet musuh, dan healthnya, Cus kita kirim ke client
            
                                if (send(socketnya, &musuh_terpilih, sizeof(musuh_terpilih), 0) <= 0) {
                                    perror("send failed (musuh)");
                                    break; // Keluar dari perulangan jika terjadi error
                                }
                                if (send(socketnya, &health_musuh, sizeof(health_musuh), 0) <= 0) {
                                    perror("send failed (health musuh)");
                                    break; // Keluar dari perulangan jika terjadi error
                                }

                                // Tambah uang player
                                int uang_dapat = (rand() % 41) + 20; // Uang dapat antara 20 hingga 60
                                player[nomor_player].uang += uang_dapat;
                                if (send(socketnya, &uang_dapat, sizeof(uang_dapat), 0) <= 0) {
                                    perror("send failed (uang dapat)");
                                    break; // Keluar dari perulangan jika terjadi error
                                }

                                // Kirim health player ke client
                                if (send(socketnya, &player[nomor_player].darah, sizeof(player[nomor_player].darah), 0) <= 0) {
                                    perror("send failed (health player)");
                                    break; // Keluar dari perulangan jika terjadi error
                                }
                                printf("Musuh dan healthnya dikirim ke client dengan port %d\n", client_port);
                            } else {
                                if (send(socketnya, &health_musuh, sizeof(health_musuh), 0) <= 0) {
                                    perror("send failed (health musuh)");
                                    break; // Keluar dari perulangan jika terjadi error
                                }

                                printf("Health musuh sekarang: %d\n", health_musuh);

                                // Musuh menyerang balik
                                int damage_musuh = rand() % (musuh_terpilih.damage + 1); // Random damage antara 0 hingga damage maksimum musuh
                                player[nomor_player].darah -= damage_musuh;

                                // Kirim health player ke client
                                if (send(socketnya, &player[nomor_player].darah, sizeof(player[nomor_player].darah), 0) <= 0) {
                                    perror("send failed (health player)");
                                    break; // Keluar dari perulangan jika terjadi error
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
                    }

                } else {
                    printf("Player dengan port %d tidak ditemukan.\n", client_port);
                }
                break;
            }
            default:
                printf("Pilihan tidak valid dari client dengan port %d: %d\n", client_port, pilihan_dari_client);
                break;
        }
    }

    // Hapus client dari array saat koneksi selesai
    pthread_mutex_lock(&client_lock);
    for (int i = 0; i < hitung_clientnya; i++) {
        if (clients[i].port == client_port) {
            // Hapus data client
            for (int j = i; j < hitung_clientnya - 1; j++) {
                clients[j] = clients[j + 1];
                player[j] = player[j + 1];
                memcpy(inv[j], inv[j + 1], sizeof(inv[j]));
            }
            hitung_clientnya--;
            printf("Client dengan port %d dihapus. Total client: %d\n", client_port, hitung_clientnya);
            break;
        }
    }
    pthread_mutex_unlock(&client_lock);

    close(socketnya);
    return NULL;
}

int main(){
    int servernya, socketnya;
    struct sockaddr_in alamat;
    int addrlen = sizeof(alamat);
    int num1, num2, result;
    char buffer[BUFFER_SIZE] = {0};

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

    pthread_mutex_destroy(&client_lock); // Hancurkan mutex
    close(servernya);
    return 0;
}