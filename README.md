# Sisop-4-2025-IT39

# Soal 1
Dikerjakan oleh Clarissa Aydin Rahmazea (5027241014)

# Soal 2
Dikerjakan oleh Ahmad Wildan Fawwaz (5027241001)

## Pengerjaan
A. Membuat filesystem virtual FUSE
```c
static int baymax_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;

    if (strcmp(path, "/") != 0) return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    DIR *d = opendir(RELICS_DIR);
    if (!d) return -ENOENT;

    char seen[100][MAX_FILENAME] = {0};
    int seen_count = 0;

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        char *dot = strrchr(de->d_name, '.');
        if (!dot || strlen(dot) != 4) continue;

        char name[MAX_FILENAME];
        strncpy(name, de->d_name, dot - de->d_name);
        name[dot - de->d_name] = '\0';

        int found = 0;
        for (int i = 0; i < seen_count; i++) {
            if (strcmp(seen[i], name) == 0) {
                found = 1;
                break;
            }
        }

        if (!found) {
            filler(buf, name, NULL, 0, 0);
            strncpy(seen[seen_count++], name, MAX_FILENAME);
        }
    }
    closedir(d);
    return 0;
}
```


B. File dapat dibaca, disalin, dan ditampilkan utuh
```c


# Soal 3
Dikerjakan oleh Muhammad Rafi' Adly (5027241082)

## a. Sistem AntiNK

# Soal 4
Dikerjakan oleh
