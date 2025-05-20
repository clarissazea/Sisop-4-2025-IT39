# Sisop-4-2025-IT39

# Soal 1
Dikerjakan oleh Clarissa Aydin Rahmazea (5027241014)

Program hexed.c adalah implementasi virtual filesystem berbasis FUSE yang bertujuan untuk membantu Shorekeeper dalam mengelola dan menginterpretasikan anomali teks hexadecimal yang ditemukan di wilayah Black Shores. Ketika direktori /image dalam filesystem virtual dibuka, sistem akan secara otomatis mencari file teks hexadecimal di direktori sumber (anomali/), mengonversinya menjadi file gambar .png, dan mencatat proses tersebut ke dalam conversion.log.

## Alur Kerja System
```bash
- Mount FUSE:
    Sistem file virtual di-mount di <mountpoint_mnt>.
- Akses Direktori:
    - Saat user membuka /, akan melihat daftar file .txt dan direktori image.
    - Saat membuka /image, semua file .txt dikonversi ke PNG.
- Konversi Hexadecimal:
    - File teks hexadecimal dibaca per dua karakter → diubah ke byte → ditulis sebagai PNG.
- Logging:
    - Setiap konversi dicatat di conversion.log.

## Struktur File Input/Output
- Input: File teks .txt yang berada di direktori anomali/ dengan konten string heksadesimal.
- Output:
    - File gambar .png di anomali/image/ dengan nama [nama_file]_image_[timestamp].png
    - File log conversion.log di anomali/ yang mencatat waktu dan hasil konversi.
```


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

```


# Soal 3
Dikerjakan oleh Muhammad Rafi' Adly (5027241082)

## a. Sistem AntiNK

# Soal 4
Dikerjakan oleh
