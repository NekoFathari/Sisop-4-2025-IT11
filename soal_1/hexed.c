#define FUSE_USE_VERSION 28 
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#ifndef S_IFDIR
#define S_IFDIR 0040000
#endif
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define path_log "/anomali/conversion.log"

char sumber[256];
char buat_gambar[256];

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

static void fs_clear_image(){
    DIR *bukadir = opendir(buat_gambar);
    if (bukadir == NULL) {
        perror("Failed to open directory");
        return;
    }

    while (readdir(bukadir) != NULL) {
        char *file = readdir(bukadir)->d_name;
        if (strcmp(file, ".") != 0 && strcmp(file, "..") != 0) {
            char lokasi_filenya[256];
            snprintf(lokasi_filenya, sizeof(lokasi_filenya), "%s/%s", buat_gambar, file);
            if (remove(lokasi_filenya) == 0) {
                printf("Deleted file: %s\n", lokasi_filenya);
            } else {
                perror("Failed to delete file");
            }
        }
    }
    closedir(bukadir);

    // hapus logging di foldernya
    char lokasi_log[256] = "";
    strcat(lokasi_log, buat_gambar);
    strcat(lokasi_log, "/log.txt");
    if (unlink(lokasi_log) == 0) {
        printf("Deleted log file: %s\n", lokasi_log);
    } else {
        perror("Failed to delete log file");
    }
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
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    } else if (strcmp(path, "/image") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    char lokasi_filenya[256];
    snprintf(lokasi_filenya, sizeof(lokasi_filenya), "%s%s", sumber, path);
    // kita check isi dari file apakah sama?
    if (strncmp(path, "/image/", 7) == 0) {
        const char *file = path + 7;
        snprintf(lokasi_filenya, sizeof(lokasi_filenya), "%s/image/%s", sumber, file);
        FILE *file_check = fopen(lokasi_filenya, "r");
        if (file_check == NULL) {
            return -ENOENT;
        }
        fclose(file_check);
    } else {
        FILE *file_check = fopen(lokasi_filenya, "r");
        if (file_check == NULL) {
            return -ENOENT;
        }
        fclose(file_check);
    }
    return 0;
}

// cek apkah text dan png aman kah?
static void fs_qc_duluges(const char *basisnya){
    // ambil lokasi textnya dulu, kan dia di folder anomali, dan dia ada di fungsi lokasi
    snprintf(sumber, sizeof(sumber), "%s/%s.txt", sumber, basisnya);
    
    // gimana kalau file textnya tidak ada?
    DIR *direktoriwak;
    if(access(sumber, F_OK) == 0){
        if((direktoriwak = opendir(sumber)) == NULL){
            perror("Failed to open directory");
            return;
        }
    } else {
        perror("File does not exist");
        return;
    }
    char keluar_yg_diharapkan[100];
    sprintf(keluar_yg_diharapkan, "%s_image_", sumber);
    for (int i = 0; i < 100; i++) {
        if (strncmp(keluar_yg_diharapkan, sumber, strlen(keluar_yg_diharapkan)) == 0) {
            // kita ambil file textnya
            char *file = readdir(direktoriwak)->d_name;
            if (strcmp(file, ".") != 0 && strcmp(file, "..") != 0) {
                char lokasi_filenya[256];
                snprintf(lokasi_filenya, sizeof(lokasi_filenya), "%s/%s", sumber, file);
                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                char waktu[100];
                strftime(waktu, sizeof(waktu), "%Y-%m-%d_%H:%M:%S", t);
                // kita convert ke png
                char png[256];
                snprintf(png, sizeof(png), "%s/%s_image_%s_%s.png", buat_gambar, file, file, waktu);
                fs_converter(lokasi_filenya, png);

                // kita logging
                FILE *log_file = fopen(path_log, "a");

                if (log_file != NULL) {
                    fprintf(log_file, "[%s]: Successfully converted hexadecimal text %s to %s.\n", waktu, file, strrchr(png, '/') + 1);
                    fclose(log_file);
                } else {
                    perror("Failed to open log file");
                }
            }
        }
    }
    closedir(direktoriwak);
}

static int fs_readdir(const char *lokasi, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    //kita cek apakah kita di root atau di image
    if (strcmp(lokasi, "/") == 0) {
        struct dirent *de;
        DIR *dir;
        if (access(sumber, F_OK) == 0) {
            if((dir = opendir(sumber)) == NULL){
                return -errno;
            }
        } else {
            return -ENOENT;
        }

        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        filler(buf, "image", NULL, 0);

        while ((de = readdir(dir)) != NULL) {
            if (de->d_type == DT_REG) {
                filler(buf, de->d_name, NULL, 0);
            }
        }
        closedir(dir);
        return 0;
    
    } else if (strcmp(lokasi, "/image") == 0) {
        // cek image dir ada ga
        fs_clear_image();
        DIR *dir_gambar;
        if (access(buat_gambar, F_OK) == 0) {
            dir_gambar = opendir(buat_gambar);
            if (dir_gambar == NULL) return -errno;
        } else {
            return -ENOENT;
        }
        struct dirent *de;
        DIR *dir;
        if(access(sumber, F_OK) == 0) {
            if((dir = opendir(sumber)) == NULL){
                return -errno;
            }
        } else {
            return -ENOENT;
        }
        while ((de = readdir(dir)) != NULL) {
            if (de->d_type == DT_REG && strstr(de->d_name, ".txt")) {
                char *file = de->d_name;
                char lokasi_filenya[256];
                snprintf(lokasi_filenya, sizeof(lokasi_filenya), "%s/%s", sumber, file);
                fs_qc_duluges(file);
                filler(buf, file, NULL, 0);
            }
        }
        closedir(dir);
        return 0;
    }
    return -ENOENT;
}

static int fs_open(const char *lokasinya_wak, struct fuse_file_info *fi) {
    sprintf(sumber, "%s%s", sumber, lokasinya_wak);
    // kita open wak
    int lihat;
    if(access(sumber, F_OK) == 0) {
        lihat = open(sumber, O_RDONLY);
        if (lihat == -1) {
            return -errno;
        }
    } else {
        return -ENOENT;
    }
    close(lihat);
    return 0;
}
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    sprintf(sumber, "%s%s", sumber, path);
    // kita baca
    int lihat;
    if(access(sumber, F_OK) == 0) {
        lihat = open(sumber, O_RDONLY);
        if (lihat == -1) {
            return -errno;
        }
    } else {
        return -ENOENT;
    }
    // kita baca isi filenya
    lseek(lihat, offset, SEEK_SET);
    int retired = read(lihat, buf, size);
    if (retired == -1) {
        return -errno;
    }

    close(lihat);
    return retired;

}
static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    sprintf(sumber, "%s%s", sumber, path);
    int fd;
    if (access(sumber, F_OK) == 0) {
        fd = open(sumber, O_WRONLY | O_APPEND);
        if (fd == -1) {
            return -errno;
        }
    } else {
        fd = open(sumber, O_WRONLY | O_CREAT, 0644);
        if (fd == -1) {
            return -errno;
        }
    }   
    lseek(fd, offset, SEEK_SET);
    ssize_t written = write(fd, buf, size);
    close(fd);
    
    if (written < 0) {
        return -errno;
    }
    return written;
}

static struct fuse_operations ops = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open    = fs_open,
    .read    = fs_read,
    .write   = fs_write
};


int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "--download") == 0) {
        const char *url = "https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download";
        printf("Downloading from: %s\n", url);

        // Fork for downloading
        pid_t pid_download = fork();
        if (pid_download == 0) {
            // Child process for wget
            const char *args[] = {"wget", "-O", "anomali.zip", url, NULL};
            if (execvp(args[0], (char *const *)args) == -1) {
            perror("execvp failed (wget)");
                exit(EXIT_FAILURE);
            }
        } else if (pid_download < 0) {
            perror("Fork failed (wget)");
            exit(EXIT_FAILURE);
        } else {
            // Parent process waits for download to complete
            wait(NULL);

            // Fork for extraction
            pid_t pid_unzip = fork();
            printf("The file 'anomali.zip' already exists. Skipping download and extraction.\n");
                return 0;
            } 

            // Fork for extraction
            pid_t pid_unzip = fork();
            if (pid_unzip == 0) {
            // Child process for unzip
            const char *args_unzip[] = {"unzip", "anomali.zip", NULL};
            if (execvp(args_unzip[0], (char *const *)args_unzip) == -1) {
                perror("execvp failed (unzip)");
                exit(EXIT_FAILURE);
            }
            } else if (pid_unzip < 0) {
            perror("Fork failed (unzip)");
            exit(EXIT_FAILURE);
            } else {
            // Parent process waits for extraction to complete
            wait(NULL);

            // Fork for hexed
            pid_t pid_hexed = fork();
            if (pid_hexed == 0) {
                // Child process for hexed
                const char *args_hexed[] = {"./hexed", NULL};
                if (execvp(args_hexed[0], (char *const *)args_hexed) == -1) {
                perror("execvp failed (hexed)");
                exit(EXIT_FAILURE);
                }
            } else if (pid_hexed < 0) {
                perror("Fork failed (hexed)");
                exit(EXIT_FAILURE);
            } else {
                // Parent process waits for hexed to complete
                wait(NULL);
            }
            
        }
    } else {
        // kita gunakan FUSE untuk mengakses file anomali
        // set pathnya ke folder anomali
        char *lokasi_fix = "anomali";
        realpath(lokasi_fix, sumber);

        //cek apakah folder anomali ada folder image atau tidak
        char *lokasi_gambar = strcat(sumber, "/image");
        if (access(lokasi_gambar, F_OK) == 0) {
            // folder image ada
            realpath(lokasi_gambar, buat_gambar);
        } else {
            // folder image tidak ada
            mkdir(lokasi_gambar, 0777);
        }
        loading(100);
        printf("Mounting...\n");
        if (fuse_main(argc, argv, &ops, NULL) == -1) {
            perror("Error mounting FUSE");
            return EXIT_FAILURE;
        }
        printf("Unmounting...\n");
        loading(100);
        // Unmounting is handled automatically by FUSE; no need for manual unmounting here.
        printf("Unmounting handled by FUSE.\n");
        printf("Unmounted successfully.\n");
    }
}
