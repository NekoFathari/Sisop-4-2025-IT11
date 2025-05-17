#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>

#define PORT 8001
#define IP "127.0.0.1"

// Perwarnaan wak
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

// Deklarasi global
int sock = 0;
struct sockaddr_in serv_addr;


typedef struct {
    int uang;
    int damage;
    int darah;
    int berapa_mati;
    int crit;
    int crit_damage;
    char senjata[50];
} PlayerStats;

typedef struct {
    int harga;
    int damage;
    int crit;
    int crit_damage;
    char nama[50];
} Senjatanya;

typedef struct {
    int id;
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

void choice(int pilihannya);

void logo(){
printf("\033[H\033[J");
printf("██████╗░██╗░░░██╗███╗░░██╗░██████╗░███████╗░█████╗░███╗░░██╗\n");
printf("██╔══██╗██║░░░██║████╗░██║██╔════╝░██╔════╝██╔══██╗████╗░██║\n");
printf("██║░░██║██║░░░██║██╔██╗██║██║░░██╗░█████╗░░██║░░██║██╔██╗██║\n");
printf("██║░░██║██║░░░██║██║╚████║██║░░╚██╗██╔══╝░░██║░░██║██║╚████║\n");
printf("██████╔╝╚██████╔╝██║░╚███║╚██████╔╝███████╗╚█████╔╝██║░╚███║\n");
printf("╚═════╝░░╚═════╝░╚═╝░░╚══╝░╚═════╝░╚══════╝░╚════╝░╚═╝░░╚══╝\n\n");


}

void loading(int berapa){
    printf("                         Loading...\n");
    int banyak = 60, muncul;
    printf("0 ─────────────────────────────────────────────────────── 100\n");
    muncul = (banyak * berapa) / 100;
    for (int i = 0; i < muncul; i++) {
        printf("█");
        fflush(stdout); // Ensure the output is flushed immediately
        usleep(100000); // Add a delay of 100ms for each character
    }
    printf("\n\n");
}

void health_barwak(int darah_sekarang, int darah_maksimal) {
    int banyak = 60, muncul;
    printf("0 ─────────────────────────────────────────────────────── %d\n", darah_maksimal);
    muncul = (banyak * darah_sekarang) / darah_maksimal;
    for (int i = 0; i < muncul; i++) {
        // munculkan langsung semua
        printf("█");
    }
    for (int i = muncul; i < banyak; i++) {
        // munculkan semua
        printf("░");
    }
    printf(" (%d)\n", darah_sekarang);
}

// Fungsi gamenya
void stats() {
    // Kirim permintaan stats ke server
    choice(1);

    // Terima data stats dari server
    PlayerStats stats;
    if (recv(sock, &stats, sizeof(stats), 0) <= 0) {
        perror("Receive failed (stats)");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("\033[H\033[J");
    logo();
    printf("\n");
    printf("█▀ ▀█▀ ▄▀█ ▀█▀ █▀\n");
    printf("▄█ ░█░ █▀█ ░█░ ▄█\n\n");

    // Print all stats
    printf("Uang       : %d\n", stats.uang);
    printf("Senjata    : %s\n", stats.senjata[0] ? stats.senjata : "None");
    printf("Base Damage: %d\n", stats.damage);
    printf("Darah      : %d\n", stats.darah);
    printf("Terbunuh   : %d\n", stats.berapa_mati);
    printf("Crit       : %d\n", stats.crit);
    printf("Crit Damage: %d\n", stats.crit_damage);
}

void inventori() {
    // Kirim permintaan inventori ke server
    choice(2);

    // Terima data inventory dari server
    Inventory inventory[20];
    if (recv(sock, &inventory, sizeof(inventory), 0) <= 0) {
        perror("Receive failed (inventory)");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("\033[H\033[J");
    logo();
    printf("\n");
    printf("█ █▄░█ █▀█ █▄█\n");
    printf("█ █░▀█ █▄█ ░█░\n\n");

    printf("Inventory Anda:\n");
    for (int i = 0; i < 20; i++) {
        if (inventory[i].id != 0) {
            printf("Slot %d - ID: %d, Nama: %s, Harga Jual: %d, Banyaknya: %d\n",
                   i + 1,
                   inventory[i].id,
                   inventory[i].nama,
                   inventory[i].harga_jual,
                   inventory[i].banyaknya);
        }
    }

    // Jika inventory ada
    printf("\n");
    printf("Pilih ID yang ingin digunakan (1-20) atau 0 untuk tidak memakai apapun: ");
    int slot;
    scanf("%d", &slot);
    if (slot < 0 || slot > 20) {
        printf("Pilihan tidak valid. Kembali ke menu.\n");
        return;
    }
    if (send(sock, &slot, sizeof(slot), 0) < 0) {
        perror("Send failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    // Terima response
    char response[100];
    if (recv(sock, response, sizeof(response), 0) <= 0) {
        perror("Receive failed (response)");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("%s\n", response);
    printf("\nPress Enter to continue...");
    getchar();
    getchar(); // Tunggu input Enter dari user
}

void shop() {
    choice(3);

    // Terima data senjata dari server
    Senjatanya senjata[5];

    if (recv(sock, senjata, sizeof(senjata), 0) <= 0) {
        perror("Receive failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    

    // Terima data uang dari server
    int uang;
    if (recv(sock, &uang, sizeof(uang), 0) <= 0) {
        perror("Receive failed (uang)");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("\033[H\033[J");
    logo();
    printf("\n");
    printf("█▀ █░█ █▀█ █▀█\n");
    printf("▄█ █▀█ █▄█ █▀▀\n\n");
    
    printf("Uang Anda: %d\n", uang);
    printf("Senjata yang tersedia:\n");
    for (int i = 0; i < 5; i++) {
        printf("[%d] %s\n", i + 1, senjata[i].nama);
        printf("    Harga: %d, Damage: %d", senjata[i].harga, senjata[i].damage);
        if (senjata[i].crit != 0 && senjata[i].crit_damage != 0) {
            printf(", Passive (Crit: %d, Crit Damage: %d)", senjata[i].crit, senjata[i].crit_damage);
        }
        printf("\n");
    }
    printf("\n");
    printf("Pilih senjata yang ingin dibeli (1-5) atau 0 untuk batal: ");
    int pilihan_senjata;
    scanf("%d", &pilihan_senjata);

    if (pilihan_senjata < 0 || pilihan_senjata > 5) {
        printf("Pilihan tidak valid. Kembali ke menu.\n");
        return;
    }
    if (send(sock, &pilihan_senjata, sizeof(pilihan_senjata), 0) < 0) {
        perror("Send failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Terima response
    char response[100];
    if (recv(sock, response, sizeof(response), 0) <= 0) {
        perror("Receive failed (response)");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("%s\n", response);
}

void battle() {
    // Kirim permintaan battle ke server
    choice(4);

    musuhnya musuhku;

    // Terima data musuh dari server
    if (recv(sock, &musuhku, sizeof(musuhku), 0) <= 0) {
        perror("Receive failed (musuh)");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Terima health musuh dari server
    int health_musuh_max;
    if (recv(sock, &health_musuh_max, sizeof(health_musuh_max), 0) <= 0) {
        perror("Receive failed (health musuh)");
        close(sock);
        exit(EXIT_FAILURE);
    }

    int health_musuh = health_musuh_max;
    int pilihan_battle;

    do {
        printf("\033[H\033[J"); // Clear screen
        logo();
        printf("%s\n", musuhku.nama);
        if (health_musuh_max >= health_musuh) {
            health_barwak(health_musuh, health_musuh_max);
        }

        printf("Pilih aksi (1 untuk menyerang, 2 untuk kabur): ");
        scanf("%d", &pilihan_battle);

        if (pilihan_battle < 1 || pilihan_battle > 2) {
            printf("Pilihan tidak valid. Silakan coba lagi.\n");
            continue;
        }

        if (send(sock, &pilihan_battle, sizeof(pilihan_battle), 0) < 0) {
            perror("Send failed");
            close(sock);
            exit(EXIT_FAILURE);
        }

        if (pilihan_battle == 2) {
            printf("Anda memilih untuk kabur. Kembali ke menu.\n");
            break;
        }

        // Terima health musuh yang diperbarui dari server
        if (recv(sock, &health_musuh, sizeof(health_musuh), 0) <= 0) {
            perror("Receive failed (health musuh)");
            close(sock);
            exit(EXIT_FAILURE);
        }

        if (health_musuh <= 0) {
            printf("Musuh telah dikalahkan!\n");
            sleep(2);

            // Terima musuh yang baru dari server
            if (recv(sock, &musuhku, sizeof(musuhku), 0) <= 0) {
                perror("Receive failed (musuh baru)");
                close(sock);
                exit(EXIT_FAILURE);
            }
            // Terima health musuh baru dari server
            if (recv(sock, &health_musuh_max, sizeof(health_musuh_max), 0) <= 0) {
                perror("Receive failed (health musuh baru)");
                close(sock);
                exit(EXIT_FAILURE);
            }
            health_musuh = health_musuh_max;
            // Dapet notif uang
            int uang_dapat;
            if (recv(sock, &uang_dapat, sizeof(uang_dapat), 0) <= 0) {
                perror("Receive failed (uang dapat)");
                close(sock);
                exit(EXIT_FAILURE);
            }
            printf("Anda mendapatkan %d uang!\n", uang_dapat);
            sleep(2);
            printf("Musuh baru muncul!: %s\n", musuhku.nama);
            sleep(2);
        }

        // Terima darah pemain yang diperbarui dari server
        int darahku;
        if (recv(sock, &darahku, sizeof(darahku), 0) <= 0) {
            perror("Receive failed (darahku)");
            close(sock);
            exit(EXIT_FAILURE);
        }

        if(darahku <= 0) {
            printf("Anda telah kalah dalam pertarungan. Kembali ke menu.\n");
            break;
        }
        printf("Darah Anda: (%d/%d)\n", darahku, 100);
        sleep(2);

    } while (pilihan_battle != 2);
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
        case 1:
            stats();
            // tunggu sampai server ngirim balasan
            printf("\nPress Enter to continue...");
            getchar();
            getchar(); // Tunggu input Enter dari user
            menu();
            break;
        case 2:
            inventori();
            menu();
            break;
        case 3:
            shop();
            printf("\nPress Enter to continue...");
            getchar();
            getchar(); // Tunggu input Enter dari user
            menu();
            break;
        case 4:
            battle();
            sleep(1);
            menu();
            break;
        case 5:
            printf("Exiting game...\n");
            exit(0);
        default:
            printf("\033[1;31mInvalid choice. Please try again.\033[0m\n");
            sleep(2);
            menu();
    }

}    

void choice(int pilihannya){
    if (send(sock, &pilihannya, sizeof(pilihannya), 0) < 0) {
        perror("Send failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
}

int main() {
    logo();

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

    // loading(100);
    sleep(1);
    menu();
    

    // Kirim dua angka ke server
    /*
    printf("Mengirim angka: %d dan %d\n", num1, num2);
    printf("Size of num1: %zu bytes\n", sizeof(num1));
    printf("Size of num2: %zu bytes\n", sizeof(num2));
    
    if (send(sock, &num1, sizeof(num1), 0) < 0) {
        perror("Send failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    if (send(sock, &num2, sizeof(num2), 0) < 0) {
        perror("Send failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    // Terima hasil dari server
    ssize_t bytes_received = recv(sock, &result, sizeof(result), 0);
    if (bytes_received < 0) {
        perror("Receive failed");
        close(sock);
        exit(EXIT_FAILURE);
    } else if (bytes_received != sizeof(result)) {
        fprintf(stderr, "Error: Expected %ld bytes, but got %ld bytes\n", sizeof(result), bytes_received);
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Hasil penjumlahan: %d\n", result);
    */
}