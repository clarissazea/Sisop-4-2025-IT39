#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

static char src_dir[1024];          
static char img_dir[1024];          

static void clear_images_dir()
{
    DIR *dp = opendir(img_dir);
    if (!dp) return;

    struct dirent *de;
    char filepath[1024];
    while ((de = readdir(dp))) {
        if (de->d_type == DT_REG && strstr(de->d_name, ".png")) {
            snprintf(filepath, sizeof(filepath), "%s/%s", img_dir, de->d_name);
            unlink(filepath); 
        }
    }
    closedir(dp);

    
    char log_path[1024];
    snprintf(log_path, sizeof(log_path), "%s/conversion.log", src_dir);
    unlink(log_path); // remove log file before regeneration
}


static void hex_to_png(const char *txt_path, const char *png_path)
{
    FILE *in  = fopen(txt_path, "r");
    if (!in) return;

    FILE *out = fopen(png_path, "wb");
    if (!out) { fclose(in); return; }

    int hi, lo;
    while ((hi = fgetc(in)) != EOF) {
        if (hi == '\n' || hi == '\r' || hi == ' ') continue;
        lo = fgetc(in);
        if (lo == EOF) break;
        char hex[3] = { (char)hi, (char)lo, 0 };
        unsigned char byte = (unsigned char) strtol(hex, NULL, 16);
        fputc(byte, out);
    }
    fclose(in);
    fclose(out);
}

static void ensure_png(const char *base)          
{
    
    char txt[1024], png[1024];
    snprintf(txt, sizeof(txt), "%s/%s.txt", src_dir, base);

    struct stat st;
    if (stat(txt, &st) == -1) return;             

    DIR *dp = opendir(img_dir);
    if (!dp) return;
    struct dirent *de;
    char wanted_prefix[256];
    snprintf(wanted_prefix, sizeof(wanted_prefix), "%s_image_", base);
    while ((de = readdir(dp))) {
        if (strncmp(de->d_name, wanted_prefix, strlen(wanted_prefix)) == 0) {
            closedir(dp);                        
            return;
        }
    }
    closedir(dp);

    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d_%H:%M:%S", t);
    snprintf(png, sizeof(png), "%s/%s_image_%s.png", img_dir, base, ts);

    hex_to_png(txt, png);

    
    char log_path[1024];
    sprintf(log_path, "%s/conversion.log", src_dir); 

    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H:%M:%S", tm_info);

    FILE *log_file = fopen(log_path, "a");
    if (log_file) {
        fprintf(log_file, "[%s]: Successfully converted hexadecimal text %s to %s.\n",
                timestamp, base, strrchr(png, '/') + 1);
        fclose(log_file);
    }
}

static int fs_getattr(const char *path, struct stat *st)
{
    memset(st, 0, sizeof(*st));

    
    if (strcmp(path, "/image") == 0 || strcmp(path, "/image/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }

    char real[1024];
    if (strncmp(path, "/image/", 8) == 0)        
        snprintf(real, sizeof(real), "%s/%s", img_dir, path + 8);
    else                                         
        snprintf(real, sizeof(real), "%s%s", src_dir, path);

    if (lstat(real, st) == -1) return -errno;
    return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t off, struct fuse_file_info *fi)
{
    (void) off; (void) fi;

    
    if (strcmp(path, "/") == 0) {
        filler(buf, ".",  NULL, 0);
        filler(buf, "..", NULL, 0);
        filler(buf, "image", NULL, 0);
        filler(buf, "conversion.log", NULL, 0);

        DIR *dp = opendir(src_dir);
        if (!dp) return -errno;
        struct dirent *de;
        while ((de = readdir(dp))) {
            if (de->d_type == DT_REG && strstr(de->d_name, ".txt"))
                filler(buf, de->d_name, NULL, 0);
        }
        closedir(dp);
        return 0;
    }

    
    if (strcmp(path, "/image") == 0 || strcmp(path, "/image/") == 0) {
        filler(buf, ".",  NULL, 0);
        filler(buf, "..", NULL, 0);

        clear_images_dir();

    
        DIR *dp_txt = opendir(src_dir);
        if (dp_txt) {
            struct dirent *de;
            while ((de = readdir(dp_txt))) {
                if (de->d_type == DT_REG && strstr(de->d_name, ".txt")) {
                    char base[256];
                    strncpy(base, de->d_name, sizeof(base));
                    base[strlen(base)-4] = '\0';   
                    ensure_png(base);
                }
            }
            closedir(dp_txt);
        }
    
        DIR *dp = opendir(img_dir);
        if (!dp) return -errno;
        struct dirent *de;
        while ((de = readdir(dp))) {
            if (de->d_type == DT_REG && strstr(de->d_name, ".png"))
                filler(buf, de->d_name, NULL, 0);
        }
        closedir(dp);
        return 0;
    }
    return -ENOENT;
}

static int fs_open(const char *path, struct fuse_file_info *fi)
{
    char real[1024];
    if (strncmp(path, "/image/", 8) == 0)
        snprintf(real, sizeof(real), "%s/%s", img_dir, path + 8);
    else
        snprintf(real, sizeof(real), "%s%s", src_dir, path);

    int fd = open(real, O_RDONLY);
    if (fd == -1) return -errno;
    close(fd);
    return 0;
}

static int fs_read(const char *path, char *buf, size_t sz,
                   off_t off, struct fuse_file_info *fi)
{
    (void) fi;
    char real[1024];
    if (strncmp(path, "/image/", 8) == 0)
        snprintf(real, sizeof(real), "%s/%s", img_dir, path + 8);
    else
        snprintf(real, sizeof(real), "%s%s", src_dir, path);

    int fd = open(real, O_RDONLY);
    if (fd == -1) return -errno;
    int rd = pread(fd, buf, sz, off);
    if (rd == -1) rd = -errno;
    close(fd);
    return rd;
}

static struct fuse_operations ops = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open    = fs_open,
    .read    = fs_read,
};

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr,
            "Usage: %s <mountpoint_mnt> [FUSE opts]\n",
            argv[0]);
        return 1;
    }

    
    realpath("anomali", src_dir);  
    snprintf(img_dir, sizeof(img_dir), "%s/image", src_dir);
    mkdir(img_dir, 0755);  
    
    
    argv[1] = argv[1];  
 

    umask(0);
    return fuse_main(argc, argv, &ops, NULL);
}
