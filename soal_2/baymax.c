#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define RELIC_DIR "relics"
#define LOG_FILE "activity.log"
#define CHUNK_SIZE 1024
#define MAX_CHUNKS 1000

static const char *virtual_file = "Baymax.jpeg";

// Logging helper
void write_log(const char *action, const char *info) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char ts[64];
    strftime(ts, sizeof(ts), "[%Y-%m-%d %H:%M:%S]", t);
    fprintf(log, "%s %s: %s\n", ts, action, info);
    fclose(log);
}

// Menyediakan isi direktori
static int baymax_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;

    if (strcmp(path, "/") != 0) return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, virtual_file, NULL, 0, 0);
    return 0;
}

// Mendapatkan atribut file
static int baymax_getattr(const char *path, struct stat *st,
                         struct fuse_file_info *fi) {
    (void) fi;
    memset(st, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else if (strcmp(path + 1, virtual_file) == 0) {
        st->st_mode = S_IFREG | 0444;
        st->st_nlink = 1;

        // Hitung ukuran gabungan file
        off_t size = 0;
        char chunk_path[256];
        for (int i = 0; i < 14; ++i) {
            snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_DIR, virtual_file, i);
            FILE *fp = fopen(chunk_path, "rb");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                size += ftell(fp);
                fclose(fp);
            }
        }
        st->st_size = size;
    } else {
        return -ENOENT;
    }
    return 0;
}

// Membaca isi file virtual
static int baymax_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    (void) fi;
    if (strcmp(path + 1, virtual_file) != 0)
        return -ENOENT;

    write_log("READ", virtual_file);

    size_t bytes_read = 0;
    off_t current_offset = 0;
    char chunk_path[256];
    for (int i = 0; i < 14; ++i) {
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_DIR, virtual_file, i);
        FILE *fp = fopen(chunk_path, "rb");
        if (!fp) continue;

        fseek(fp, 0, SEEK_END);
        size_t chunk_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (offset < current_offset + chunk_size) {
            fseek(fp, offset - current_offset, SEEK_SET);
            size_t to_read = chunk_size - (offset - current_offset);
            if (to_read > size - bytes_read) to_read = size - bytes_read;
            fread(buf + bytes_read, 1, to_read, fp);
            bytes_read += to_read;
            offset += to_read;
            if (bytes_read >= size) {
                fclose(fp);
                break;
            }
        }
        current_offset += chunk_size;
        fclose(fp);
    }

    return bytes_read;
}

// Menulis file baru (akan dipecah)
static int baymax_write(const char *path, const char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi) {
    (void) offset; (void) fi;

    const char *filename = path + 1;
    char chunk_path[256];
    int chunk_count = 0;
    size_t written = 0;

    while (written < size) {
        size_t to_write = size - written > CHUNK_SIZE ? CHUNK_SIZE : size - written;
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_DIR, filename, chunk_count);
        FILE *fp = fopen(chunk_path, "wb");
        if (!fp) break;
        fwrite(buf + written, 1, to_write, fp);
        fclose(fp);
        written += to_write;
        chunk_count++;
    }

    char log_entry[512];
    snprintf(log_entry, sizeof(log_entry), "%s ->", filename);
    for (int i = 0; i < chunk_count; i++) {
        char chunk[64];
        snprintf(chunk, sizeof(chunk), " %s.%03d", filename, i);
        strcat(log_entry, chunk);
        if (i != chunk_count - 1) strcat(log_entry, ",");
    }
    write_log("WRITE", log_entry);
    return size;
}

// Menghapus file virtual (hapus semua pecahan)
static int baymax_unlink(const char *path) {
    const char *filename = path + 1;
    char chunk_path[256];
    int deleted = 0;

    for (int i = 0; i < MAX_CHUNKS; ++i) {
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_DIR, filename, i);
        if (unlink(chunk_path) == 0)
            deleted++;
        else
            break;
    }

    if (deleted > 0) {
        char log_entry[256];
        snprintf(log_entry, sizeof(log_entry), "%s.000 - %s.%03d", filename, filename, deleted - 1);
        write_log("DELETE", log_entry);
    }

    return 0;
}

static const struct fuse_operations baymax_oper = {
    .getattr = baymax_getattr,
    .readdir = baymax_readdir,
    .read = baymax_read,
    .write = baymax_write,
    .unlink = baymax_unlink,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &baymax_oper, NULL);
}
