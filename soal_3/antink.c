#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

// ROT13 encryption
void rot13(char *str) {
    for (int i = 0; str[i]; i++) {
        if (isalpha(str[i])) {
            if ((tolower(str[i]) - 'a') < 13)
                str[i] += 13;
            else
                str[i] -= 13;
        }
    }
}

// Reverse string
void reverse_str(char *str) {
    int n = strlen(str);
    for (int i = 0; i < n / 2; i++) {
        char temp = str[i];
        str[i] = str[n - i - 1];
        str[n - i - 1] = temp;
    }
}

// Check if filename contains "nafis" or "kimcun"
int is_dangerous(const char *filename) {
    return strstr(filename, "nafis") || strstr(filename, "kimcun");
}

// Log function
void log_activity(const char *action, const char *filename) {
    FILE *log_file = fopen("/var/log/it24.log", "a");
    if (log_file) {
        time_t now;
        time(&now);
        fprintf(log_file, "[%s] %s: %s\n", ctime(&now), action, filename);
        fclose(log_file);
    }
}

static int antink_getattr(const char *path, struct stat *stbuf) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "/it24__host%s", path);
    
    int res = lstat(full_path, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;
    
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "/it24__host%s", path);
    
    DIR *dp = opendir(full_path);
    if (dp == NULL) return -errno;
    
    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        
        char display_name[256];
        strcpy(display_name, de->d_name);
        
        if (is_dangerous(de->d_name)) {
            reverse_str(display_name);
            log_activity("DANGEROUS_FILE_DETECTED", de->d_name);
        }
        
        filler(buf, display_name, &st, 0);
    }
    closedir(dp);
    return 0;
}

static int antink_open(const char *path, struct fuse_file_info *fi) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "/it24__host%s", path);
    
    int res = open(full_path, fi->flags);
    if (res == -1) return -errno;
    
    close(res);
    return 0;
}

static int antink_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "/it24__host%s", path);
    
    int fd = open(full_path, O_RDONLY);
    if (fd == -1) return -errno;
    
    int res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;
    
    // Encrypt content if not dangerous file and is text file
    if (!is_dangerous(path) && strstr(path, ".txt")) {
        rot13(buf);
        log_activity("FILE_ENCRYPTED", path);
    } else {
        log_activity("FILE_ACCESSED", path);
    }
    
    close(fd);
    return res;
}

static struct fuse_operations antink_oper = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .open = antink_open,
    .read = antink_read,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &antink_oper, NULL);
}
