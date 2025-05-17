#ifndef SHOP_H
#define SHOP_H
#include <stdio.h>
#include <string.h>

// Define the Senjata structure
typedef struct {
    int harga;
    int damage;
    int crit;
    int crit_damage;
    char nama[50];
} Senjata;

Senjata senjata[5] = {
    {85, 3, 10, 0, "Pedang Biasa aja, meh"},
    {300, 5, 35, 2, "Pedang Luar Biasa"},
    {75, 4, 0, 0, "Kapak"},
    {180, 4, 0, 0, "Tongkat"},
    {250, 5, 20, 2, "Panah"}
};

void beli_senjata(int socketnya, stat_player* player, Inventory inv[20]) {
    // Contoh implementasi pembelian senjata
    // list senjata yang ada masukkan ke string

    // Kirim daftar senjata ke client sebagai satu struct array
    if (send(socketnya, senjata, sizeof(senjata), 0) < 0) {
        perror("send failed");
        return;
    }

    // Kirim uang client ke client
    if (send(socketnya, &player->uang, sizeof(player->uang), 0) < 0) {
        perror("send failed");
        return;
    }
    // Terima pilihan senjata dari client
    int pilihan_senjata;
    if (recv(socketnya, &pilihan_senjata, sizeof(pilihan_senjata), 0) <= 0) {
        perror("recv failed");
        return;
    }

    if (pilihan_senjata < 0 || pilihan_senjata >= sizeof(senjata) / sizeof(Senjata)) {
        printf("Client memasukan pilihan senjata tidak valid\n");
        send(socketnya, "Pilihan tidak valid", sizeof("Pilihan tidak valid"), 0);
        return;
    }

    if (pilihan_senjata == 0){
        printf("Clinet tidak memilih untuk tidak membeli senjata");
        send(socketnya, "Anda tidak membeli senjata", sizeof("Anda tidak membeli senjata"), 0);
        return;
    }

    // pilihan_senjata--; // Sesuaikan dengan indeks array (0-4) (Masalah disini ga bisa di kurangi, kalau indexnya 0 dibaca sudah ada isinya.. aneh)

    // Cek apakah client memiliki barang tersebut di Inventory
    for (int i = 0; i < 20; i++) {
        if (inv[i].id == pilihan_senjata || inv[i].nama == senjata[pilihan_senjata].nama) {
            printf("Client sudah memiliki senjata ini\n");
            send(socketnya, "Anda sudah memiliki senjata ini", sizeof("Anda sudah memiliki senjata ini"), 0);
            return;
        }
    }
    
    // Cek apakah client memiliki cukup uang
    if (player->uang < senjata[pilihan_senjata-1].harga) {
        printf("Uang client tidak cukup untuk membeli senjata\n");
        send(socketnya, "Uang Anda tidak cukup", sizeof("Uang Anda tidak cukup"), 0);
        return;
    } else {
        // Langsung kurangi uang client
        player->uang -= senjata[pilihan_senjata-1].harga;

        // Tambahkan senjata ke inventory client
        for (int i = 0; i < 20; i++) {
            if (inv[i].id == 0) { // Cari slot kosong
                inv[i].id = pilihan_senjata;
                inv[i].banyaknya = 1;
                inv[i].harga_jual = senjata[pilihan_senjata-1].harga - senjata[pilihan_senjata-1].harga * 0.3; // harga jual 30% lebih murah
                strcpy(inv[i].nama, senjata[pilihan_senjata-1].nama);
                break;
            }
        }

        //coba keluarkan output inv yang di isi
        printf("ID: %d, Nama: %s, Harga Jual: %d, Banyaknya: %d\n",
               inv[pilihan_senjata].id,
               inv[pilihan_senjata].nama,
               inv[pilihan_senjata].harga_jual,
               inv[pilihan_senjata].banyaknya);
        printf("Client membeli senjata %s\n", senjata[pilihan_senjata].nama);
    }

    // Kirim Respons ke client pembelian selesai
    char response[100] = {0};
    snprintf(response, sizeof(response), "Anda telah membeli %s dengan harga %d. Uang Anda sekarang %d.", senjata[pilihan_senjata-1].nama, senjata[pilihan_senjata-1].harga, player->uang);
    if (send(socketnya, response, sizeof(response), 0) < 0) {
        perror("send failed");
        return;
    }

}



#endif // SHOP_H