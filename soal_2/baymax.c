#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>

#define FILE_NAME "Baymax.jpeg"
#define PART_COUNT 14
#define PART_SIZE 1024
#define MAX_FILES 100

char relics_path[256];
const char *log_path = "/home/erlinda/Sisop-4-2025-IT11/soal_2/activity.log";

// Struktur manual untuk simpan file sementara
struct TempFile {
    char name[256];
    FILE *fp;
};

struct TempFile temp_files[MAX_FILES];
int temp_count = 0;

// ================= SUBSOAL E - Logging =================
FILE* get_temp_file(const char* name) {
    for (int i = 0; i < temp_count; i++) {
        if (strcmp(temp_files[i].name, name) == 0)
            return temp_files[i].fp;
    }
    return NULL;
}

void remove_temp_file(const char* name) {
    for (int i = 0; i < temp_count; i++) {
        if (strcmp(temp_files[i].name, name) == 0) {
            fclose(temp_files[i].fp);
            for (int j = i; j < temp_count - 1; j++) {
                temp_files[j] = temp_files[j + 1];
            }
            temp_count--;
            return;
        }
    }
}

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

// ================= SUBSOAL A - Mount FUSE + tampilkan Baymax.jpeg =================
static int do_getattr(const char *path, struct stat *st) {
    memset(st, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }

    char fname[256];
    snprintf(fname, sizeof(fname), "%s", path + 1);

    char part_path[512];
    snprintf(part_path, sizeof(part_path), "%s/%s.000", relics_path, fname);
    FILE *fp = fopen(part_path, "rb");
    if (fp) {
        fclose(fp);
        st->st_mode = S_IFREG | 0666;
        st->st_nlink = 1;
        st->st_size = 0;
        int index = 0;
        while (1) {
            snprintf(part_path, sizeof(part_path), "%s/%s.%03d", relics_path, fname, index);
            FILE *part = fopen(part_path, "rb");
            if (!part) break;
            fseek(part, 0, SEEK_END);
            st->st_size += ftell(part);
            fclose(part);
            index++;
        }
        return 0;
    }

    // Tambahan untuk support file yang baru dibuat tapi belum dipecah
    FILE *tmp = get_temp_file(fname);
    if (tmp) {
        st->st_mode = S_IFREG | 0666;
        st->st_nlink = 1;
        st->st_size = 0;
        return 0;
    }

    return -ENOENT;
}

// ================= SUBSOAL A - List isi direktori mount =================
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

// ================= SUBSOAL B + E (log read) =================
static int do_open(const char *path, struct fuse_file_info *fi) {
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);
    write_log("READ", name);
    return 0;
}

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

// ================= SUBSOAL C - Simpan file ke mount_dir =================
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

// ================= SUBSOAL C + E log write =================
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

    char log_msg[3000];
    snprintf(log_msg, sizeof(log_msg), "%s -> %s", name, part_list);
    write_log("COPY", name);
    write_log("WRITE", log_msg);


    remove_temp_file(name);
    return 0;
}

// ================= SUBSOAL D + E log delete =================
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
        write_log("DELETE", deleted_range);
    }

    return 0;
}

// ================= Register fungsi =================
static struct fuse_operations ops = {
    .getattr = do_getattr,
    .readdir = do_readdir,
    .open    = do_open,
    .read    = do_read,
    .create  = do_create,
    .write   = do_write,
    .release = do_release,
    .unlink  = do_unlink,
};

int main(int argc, char *argv[]) {
    getcwd(relics_path, sizeof(relics_path));
    strcat(relics_path, "/relics");
    return fuse_main(argc, argv, &ops, NULL);
}
