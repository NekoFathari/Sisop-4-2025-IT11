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
Ada sebuah drive yang berisi pecahan data yang bernama relics. 
a. Menampilkan hasil gabungan dari zip relics

        static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    DIR *dir = opendir(relics_path);
    if (!dir) return 0;

    struct dirent *entry;
    char seen[100][256];
    int seen_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        char *dot = strrchr(entry->d_name, '.');
        if (!dot || strlen(dot) != 4) continue;

        char base[256];
        strncpy(base, entry->d_name, dot - entry->d_name);
        base[dot - entry->d_name] = '\0';

        int already = 0;
        for (int i = 0; i < seen_count; i++) {
            if (strcmp(seen[i], base) == 0) {
                already = 1;
                break;
            }
        }

        if (!already) {
            filler(buf, base, NULL, 0);
            strcpy(seen[seen_count++], base);
        }
    }
    closedir(dir);
    return 0;
}


b. Menampilkan foto bernama Baymax.jpeg yang diambil dari pecahan dari relics.zip


        static int do_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) {
    (void) fi;
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    size_t bytes_read = 0;
    size_t part_size = 1024;
    size_t start_part = offset / part_size;
    size_t part_offset = offset % part_size;

    while (bytes_read < size) {
        char part_path[512];
        snprintf(part_path, sizeof(part_path), "%s/%s.%03zu", relics_path, name, start_part);
        FILE *fp = fopen(part_path, "rb");
        if (!fp) break;

        fseek(fp, part_offset, SEEK_SET);
        size_t to_read = part_size - part_offset;
        if (to_read > size - bytes_read)
            to_read = size - bytes_read;

        fread(buf + bytes_read, 1, to_read, fp);
        fclose(fp);

        bytes_read += to_read;
        start_part++;
        part_offset = 0;
        }

        return bytes_read;
        }

Mencatat activity.log dengan status READ

        static int do_open(const char *path, struct fuse_file_info *fi) {
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);
    write_log("READ", name); // Log aktivitas READ
    return 0;
    }

c. Membuat file baru saat masih di memory temp

        static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void)mode;
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    if (temp_count >= MAX_FILES) return -ENOMEM;

    FILE *tmp = tmpfile();
    if (!tmp) return -EIO;

    strcpy(temp_files[temp_count].name, name);
    temp_files[temp_count].fp = tmp;
    temp_count++;

    fi->fh = (uint64_t)tmp;
    return 0;
        }

Menulis ke dalam isi file temp

        static int do_write(const char *path, const char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    (void)fi;
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    FILE *tmp = get_temp_file(name);
    if (!tmp) return -ENOENT;

    fseek(tmp, offset, SEEK_SET);
    fwrite(buf, 1, size, tmp);
    return size;
        }

d. menghapus file dihapus dari direktori mount dan semua pecahan di relics juga akan ikut terhapus


        static int do_release(const char *path, struct fuse_file_info *fi) {
    (void)fi;
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    FILE *tmp = get_temp_file(name);
    if (!tmp) return 0;

    rewind(tmp);

    int part = 0;
    char buffer[1024];
    char part_list[2000] = "";

    while (!feof(tmp)) {
        size_t read_size = fread(buffer, 1, 1024, tmp);
        if (read_size <= 0) break;

        char out_path[512];
        snprintf(out_path, sizeof(out_path), "%s/%s.%03d", relics_path, name, part);
        FILE *out = fopen(out_path, "wb");
        if (out) {
            fwrite(buffer, 1, read_size, out);
            fclose(out);
        }

        char part_name[512];
        snprintf(part_name, sizeof(part_name), "%s.%03d%s", name, part, read_size == 1024 ? ", " : "");
        strcat(part_list, part_name);
        part++;
    }

    // Subsoal E: Log
    write_log("COPY", name); // Anggap semua file hasil write = hasil copy
    write_log("WRITE", part_list);

    remove_temp_file(name);
    return 0;
        
    }
Menghapus semua pecahan dan log dalam DELETE

        static int do_unlink(const char *path) {
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    char deleted_range[600];
    int last = -1;
    for (int i = 0; i < 1000; i++) {
        char part_path[512];
        snprintf(part_path, sizeof(part_path), "%s/%s.%03d", relics_path, name, i);
        if (access(part_path, F_OK) != 0) break;
        remove(part_path);
        last = i;
    }

    if (last >= 0) {
        snprintf(deleted_range, sizeof(deleted_range), "%s.%03d - %s.%03d", name, 0, name, last);
        write_log("DELETE", deleted_range); // Log DELETE
    }

    return 0;
        }

e. Hasil dari activity.log 

        void write_log(const char* action, const char* message) {
    FILE *log = fopen(log_path, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "[%Y-%m-%d %H:%M:%S]", t);

    fprintf(log, "%s %s: %s\n", time_str, action, message);
    fclose(log);
        }


### SOAL 3
