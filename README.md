# Sisop-4-2025-IT39

# Soal 1
Dikerjakan oleh Clarissa Aydin Rahmazea (5027241014)

Program `hexed.c` adalah implementasi virtual filesystem berbasis FUSE yang bertujuan untuk membantu Shorekeeper dalam mengelola dan menginterpretasikan anomali teks `hexadecimal` yang ditemukan di wilayah Black Shores. Ketika direktori /image dalam filesystem virtual dibuka, sistem akan secara otomatis mencari file teks `hexadecimal` di direktori sumber (anomali/), mengonversinya menjadi file gambar `.png`, dan mencatat proses tersebut ke dalam `conversion.log`.

## Alur Kerja System
```bash
1. Mount FUSE:
    - Sistem file virtual di-mount di `<mountpoint_mnt>`
2. Akses Direktori:
    - Saat user membuka /, akan melihat daftar file `.txt` dan direktori image.
    - Saat membuka /image, semua file `.txt` dikonversi ke PNG.
3. Konversi Hexadecimal:
    - File teks hexadecimal dibaca per dua karakter → diubah ke byte → ditulis sebagai PNG.
4. Logging:
    - Setiap konversi dicatat di `conversion.log.`
```

## Struktur File Input/Output
- Input: File teks `.txt` yang berada di direktori anomali/ dengan konten string heksadesimal.
- Output:
    - File gambar `.png` di anomali/image/ dengan nama `[nama_file]_image_[timestamp].png`
    - File log `conversion.log` di anomali/ yang mencatat waktu dan hasil konversi.

## Cara Pengerjaan

### 1. Fungsi `clear_images_dir()`

Fungsi ini bertujuan untuk membersihkan semua file PNG dalam direktori gambar sebelum proses generasi gambar baru dan menghapus log konversi sebelumnya untuk memulai log yang fresh.

```bash
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
```
Penjelasan: 

- `DIR *dp = opendir(img_dir);`: Fungsi ini membuka direktori yang ditunjuk oleh img_dir (variabel umum yang menyimpan path direktori gambar).
- `if (!dp) return;`: Jika gagal membuka direktori, fungsi akan langsung keluar.
- `struct dirent *de;
while ((de = readdir(dp)))` : Membaca setiap entri (file/subdirektori) dalam direktori
- `if (de->d_type == DT_REG && strstr(de->d_name, ".png"))`:
  - `DT_REG` memastikan itu adalah file reguler (bukan direktori/link)
  - `strstr()` memeriksa apakah nama file mengandung string ".png"
- `snprintf(log_path, sizeof(log_path), "%s/conversion.log", src_dir);
unlink(log_path);` Menghapus file log yang terletak di direktori sumber (src_dir).

### 2. Fungsi `void hex_to_png`

Fungsi ini bertujuan untuk mengkonversi data teks berformat heksadesimal (hex) ke dalam file biner PNG (decode file PNG yang sebelumnya di-encode ke format teks hex).

```bash
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
```
Penjelasan:

- `FILE *in = fopen(txt_path, "r");
if (!in) return;`
    - Membuka file teks (yang berisi data hex) untuk dibaca ("r")
    - Jika gagal membuka file, fungsi langsung keluar.
- `FILE *out = fopen(png_path, "wb");
if (!out) { fclose(in); return; }`
    - Membuka/membuat file PNG dalam mode binary write ("wb").
    - Jika gagal, file input ditutup dan fungsi keluar.
- `strtol(hex, NULL, 16)`: mengubah string hex (basis 16) menjadi nilai desimal, lalu di-cast ke `unsigned char`. Lalu tulis bytenya ke output `fputc(byte, out)` dan menulis ke file PNG.

### 3. Fungsi `ensure_png()`

Fungsi ini bertujuan untuk memastikan file PNG telah dibuat dari file teks heksadesimal (jika belum ada) dan mencatat proses konversinya ke dalam log.

```bash
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
```
Penjelasan:
- `snprintf(wanted_prefix, sizeof(wanted_prefix), "%s_image_", base);` Memeriksa apakah sudah ada file PNG dengan prefix `base_image_`
- `time_t now = time(NULL);` : Buat file PNG baru dengan timestamp
- `hex_to_png(txt, png);`: Memanggil fungsi `hex_to_png()` untuk mengubah file teks heksadesimal (txt) ke PNG (png).
- `sprintf(log_path, "%s/conversion.log", src_dir);` : Menambahkan entri log ke `src_dir/conversion.log`.
- `fprintf(log_file, "[%s]: Successfully converted %s to %s.\n", 
        timestamp, base, strrchr(png, '/') + 1);` format log sesuai dengan yang diminta soal

### 4. Fungsi `fs_getattr`

Fungsi fs_getattr() adalah bagian dari implementasi filesystem (FUSE) yang bertugas untuk mengambil metadata dari sebuah file atau direktori. Jadi fungsi ini adalah fungsi utama untuk menampilkan informasi file ke filesystem virtual.


```bash
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
```
Penjelasan:

- `memset(st, 0, sizeof(*st));`: Membersihkan struct `stat` sebelum diisi.
-
  ```
  char real[1024];
    if (strncmp(path, "/image/", 8) == 0)
    snprintf(real, sizeof(real), "%s/%s", img_dir, path + 8);
    else
    snprintf(real, sizeof(real), "%s%s", src_dir, path);
``
    - Jika path dimulai dengan `/image/` (contoh: `/image/test.png`), mapping ke direktori fisik `img_dir/test.png`.
    - Jika bukan (contoh: `/file.txt`), mapping ke `src_dir/file.txt`.

- `if (lstat(real, &st) == -1) return -errno;`: Memanggil lstat() untuk membaca atribut file fisik.


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
