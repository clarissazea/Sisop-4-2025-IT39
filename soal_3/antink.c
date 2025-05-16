#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>

#define BASE_DIR "/original"
#define LOG_FILE "/var/log/it24.log"

int nafis(const char *filename) {
    return strstr(filename, "nafis");
}

int kimcun(const char *filename) {
    return strstr(filename, "kimcun");
}

void balikAnomaly(const char *src, char *dest) {
    size_t len = strlen(src);
    for (size_t i = 0; i < len; ++i)
        dest[i] = src[len - 1 - i];
    dest[len] = '\0';
}

void encode_rot13(char *text, int len) {
    for (int i = 0; i < len; ++i) {
        if ('a' <= text[i] && text[i] <= 'z')
            text[i] = ((text[i] - 'a' + 13) % 26) + 'a';
        else if ('A' <= text[i] && text[i] <= 'Z')
            text[i] = ((text[i] - 'A' + 13) % 26) + 'A';
    }
}

void write_log(const char *msg) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(log, "[%02d-%02d-%04d %02d:%02d:%02d] %s\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec, msg);
        fclose(log);
    }
}

void fullpath(char *dest, const char *path) {
    sprintf(dest, "%s%s", BASE_DIR, path);
}

static int pujo_getattr(const char *path, struct stat *stbuf) {
    char full[1024];
    fullpath(full, path);
    int res = lstat(full, stbuf);
    return res == -1 ? -errno : 0;
}

static int pujo_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;
    (void) offset;
    (void) fi;

    char full[1024];
    fullpath(full, path);
    dp = opendir(full);
    if (dp == NULL) return -errno;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char display_name[512];
        if (nafis(de->d_name)) {
            balikAnomaly(de->d_name, display_name);
            char logmsg[1024];
            sprintf(logmsg, "[WARNING] Detected anomaly nafis : %s", de->d_name);
            write_log(logmsg);
        } else if (kimcun(de->d_name)) {
            balikAnomaly(de->d_name, display_name);
            char logmsg[1024];
            sprintf(logmsg, "[WARNING] Detected anomaly kimcun : %s", de->d_name);
            write_log(logmsg);
        } else {
            strcpy(display_name, de->d_name);
        }

        if (filler(buf, display_name, &st, 0)) break;
    }
    closedir(dp);
    return 0;
}

static int pujo_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd, res;
    (void) fi;

    char full[1024];
    fullpath(full, path);
    fd = open(full, O_RDONLY);
    if (fd == -1) return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;

    close(fd);

    if (nafis(path)) {
        char logmsg[1024];
        sprintf(logmsg, "[INFO] File read: %s (no ROT13 - anomaly nafis jir)", path);
        write_log(logmsg);
    } else if (kimcun(path)) {
        char logmsg[1024];
        sprintf(logmsg, "[INFO] File read: %s (no ROT13 - anomaly kimcun jir)", path);
        write_log(logmsg);
    } else {
        encode_rot13(buf, res);
        char logmsg[1024];
        sprintf(logmsg, "[INFO] File read: %s (ROT13 applied)", path);
        write_log(logmsg);
    }

    return res;
}

static struct fuse_operations pujo_oper = {
    .getattr = pujo_getattr,
    .readdir = pujo_readdir,
    .read = pujo_read,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &pujo_oper, NULL);
}
