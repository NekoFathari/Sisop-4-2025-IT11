# Sisop-4-2025-IT11

### MEMBER
1. Muhammad Ardiansyah Tri Wibowo - 5027241091
2. Erlinda Annisa Zahra Kusuma - 5027241108
3. Fika Arka Nuriyah - 5027241071


### REPORTING 

### SOAL 1
a. Pertama, Shorekeeper akan mengambil beberapa sampel anomali teks dari link berikut. Pastikan file zip terhapus setelah proses unzip.

Pada tahap ini kita harus mendapatkan zip, lalu unzip, dan hapus zip tersebut
hexed.c
```
....
const char *args[] = {"wget", "-O", "anomali.zip", url, NULL};
if (execvp(args[0], (char *const *)args) == -1) {
perror("execvp failed (wget)");
    exit(EXIT_FAILURE);
}
....
const char *args_unzip[] = {"unzip", "anomali.zip", NULL};
if (execvp(args_unzip[0], (char *const *)args_unzip) == -1) {
    perror("execvp failed (unzip)");
    exit(EXIT_FAILURE);
}
....
const char *args_unzip[] = {"unzip", "anomali.zip", NULL};
if (execvp(args_unzip[0], (char *const *)args_unzip) == -1) {
    perror("execvp failed (unzip)");
    exit(EXIT_FAILURE);
}
....
```

b. Setelah melihat teks - teks yang didapatkan, ia menyadari bahwa format teks tersebut adalah hexadecimal. Dengan informasi tersebut, Shorekeeper mencoba untuk mencoba idenya untuk mencari makna dari teks - teks acak tersebut, yaitu dengan mengubahnya dari string hexadecimal menjadi sebuah file image. Bantulah Shorekeeper dengan membuat kode untuk FUSE yang dapat mengubah string hexadecimal menjadi sebuah gambar ketika file text tersebut dibuka di mount directory. Lalu, letakkan hasil gambar yang didapat ke dalam directory bernama “image”.

Pada tahap ini tentu perlu banyak step salah satu paling simplenya kita harus memakai FUSE
hexed.c
```
// int main
if (access(lokasi_gambar, F_OK) == 0) {
    // folder image ada
    realpath(lokasi_gambar, buat_gambar);
} else {
    // folder image tidak ada
    mkdir(lokasi_gambar, 0777);
}
....
if (fuse_main(argc, argv, &ops, NULL) == -1) {
    perror("Error mounting FUSE");
    return EXIT_FAILURE;
}
```

hexed.c
```
// fungsi FUSE
static void fs_clear_image(){
......
} 

// convert dari heximal ke png
static void fs_converter(const char *textny_dimana, const char *textny_dituju) {
    FILE *file = fopen(textny_dimana, "r");
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }

    FILE *output = fopen(textny_dituju, "wb");
    if (output == NULL) {
        perror("Failed to open output file");
        fclose(file);
        return;
    }

    char buffer[2];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        if (buffer[0] == '\n' || buffer[0] == '\r' || buffer[0] == '\0') { 
            continue; 
        }
        unsigned char byte = (unsigned char)strtol(buffer, NULL, 16);
        fputc(byte, output);
    }
    fclose(file);
    fclose(output);
}

static int fs_getattr(const char *path, struct stat *stbuf) {
......
}

// cek apkah text dan png aman kah?
static void fs_qc_duluges(const char *basisnya){
......
}

static int fs_readdir(const char *lokasi, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
......
}

static int fs_open(const char *lokasinya_wak, struct fuse_file_info *fi) {
......
}
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
......
}
static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
......
}

static struct fuse_operations ops = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open    = fs_open,
    .read    = fs_read,
    .write   = fs_write
};

```

c. Untuk penamaan file hasil konversi dari string ke image adalah [nama file string]_image_[YYYY-mm-dd]_[HH:MM:SS].

hexed.c
```
// Pada Fungsi fs_qc_duluges
.....
time_t now = time(NULL);
struct tm *t = localtime(&now);
char waktu[100];
strftime(waktu, sizeof(waktu), "%Y-%m-%d_%H:%M:%S", t);
// kita convert ke png
char png[256];
snprintf(png, sizeof(png), "%s/%s_image_%s_%s.png", buat_gambar, file, file, waktu);
fs_converter(lokasi_filenya, png);
.....
```

d. Catat setiap konversi yang ada ke dalam sebuah log file bernama conversion.log. Untuk formatnya adalah sebagai berikut.

hexed.c
```
// Pada Fungsi fs_qc_duluges
FILE *log_file = fopen(path_log, "a");

if (log_file != NULL) {
    fprintf(log_file, "[%s]: Successfully converted hexadecimal text %s to %s.\n", waktu, file, strrchr(png, '/') + 1);
    fclose(log_file);
} else {
    perror("Failed to open log file");
}
```

REVISI SECTION
hexed.c BEFORE
```
// Child process for hexed
const char *args_hexed[] = {"./hexed", NULL};
if (execvp(args_hexed[0], (char *const *)args_hexed) == -1) {
perror("execvp failed (hexed)");
exit(EXIT_FAILURE);
}
```

hexed.c AFTER
```
// Child process for removing anomali.zip
const char *args_rm[] = {"rm", "anomali.zip", NULL};
if (execvp(args_rm[0], (char *const *)args_rm) == -1) {
    perror("execvp failed (rm)");
    exit(EXIT_FAILURE);
}
```
### SOAL 2
### SOAL 3
