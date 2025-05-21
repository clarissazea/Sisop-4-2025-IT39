# Sisop-4-2025-IT39

# Soal 1
Dikerjakan oleh Clarissa Aydin Rahmazea (5027241014)

Program `hexed.c` adalah implementasi virtual filesystem berbasis FUSE yang bertujuan untuk membantu Shorekeeper dalam mengelola dan menginterpretasikan anomali teks `hexadecimal` yang ditemukan di wilayah Black Shores. Ketika direktori /image dalam filesystem virtual dibuka, sistem akan secara otomatis mencari file teks `hexadecimal` di direktori sumber `(anomali/)`, mengonversinya menjadi file gambar `.png`, dan mencatat proses tersebut ke dalam `conversion.log`.

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

Virtual Filesystem ini membuat `/image` seolah-olah direktori nyata, meskipun tidak ada di disk.

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
```
- Jika path dimulai dengan `/image/` (contoh: `/image/test.png`), mapping ke direktori fisik `img_dir/test.png`.
- Jika bukan (contoh: `/file.txt`), mapping ke `src_dir/file.txt`.

- `if (lstat(real, &st) == -1) return -errno;`: Memanggil lstat() untuk membaca atribut file fisik.


### 5. Fungsi `fs_readdir`

Fungsi ini merupakan bagian penting dari implementasi FUSE yang bertanggung jawab untuk:
- Menampilkan daftar file/direktori saat user menjalankan perintah seperti `ls`
- Mengelola konversi otomatis file teks `.txt` ke gambar `.png`
- Menyediakan tampilan direktori virtual `/image` yang berisi hasil konversi

```
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

```
Penjelasan:
```
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t off, struct fuse_file_info *fi)
```
- `path` : Path direktori yang ingin dibaca
- `buf`: Buffer untuk menyimpan hasil
- `filler`: Fungsi callback untuk mengisi buffer dengan entri direktori

```
if (strcmp(path, "/") == 0) {
    filler(buf, ".",  NULL, 0);    // `Entri direktori saat ini`
    filler(buf, "..", NULL, 0);    // `Entri parent directory`
    filler(buf, "image", NULL, 0); // `Direktori image virtual`
    filler(buf, "conversion.log", NULL, 0); // `File log`

```
```
while ((de = readdir(dp))) {
    if (de->d_type == DT_REG && strstr(de->d_name, ".png"))
        filler(buf, de->d_name, NULL, 0);
}
```
Menampilkan file PNG hasil konversi:

Return Value
- `0` jika sukses
- `-errno` jika terjadi error saat membuka direktori
- `ENOENT` jika path tidak dikenali

### 6. Fungsi `fs_open`
Fungsi ini bertugas untuk membuka file dalam filesystem virtual sebelum operasi baca dilakukan.

```
if (strncmp(path, "/image/", 8) == 0)
    snprintf(real, sizeof(real), "%s/%s", img_dir, path + 8);
else
    snprintf(real, sizeof(real), "%s%s", src_dir, path);
```
- Jika path dimulai dengan `/image/`, mapping ke direktori fisik `img_dir`
- Jika tidak, mapping ke direktori `src_dir`
  
```
int fd = open(real, O_RDONLY);
```
- Membuka file dalam mode read-only `(O_RDONLY)`.
```
if (fd == -1) return -errno;
```
- Jika gagal, return kode error sistem

```
close(fd);
return 0;
```
- File langsung ditutup setelah dibuka (karena hanya pengecekan)
- Return 0 jika sukses

### 7. Fungsi `fs_read`
Fungsi ini bertugas untuk membaca konten file dari filesystem virtual.

```
int rd = pread(fd, buf, sz, off);
```
Operasi pembacaan:
- Membaca data dengan pread (baca pada offset tertentu)
- Parameter:
      - `buf`: Buffer penyimpanan hasil baca
      - `sz`: Jumlah byte yang ingin dibaca
      - `off`: Offset mulai membaca
- `if (rd == -1) rd = -errno;`: Penanganan error. Jika gagal, konversi error ke kode FUSE
```
static struct fuse_operations ops = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open    = fs_open,
    .read    = fs_read,
};
```
- `fs_getattr`: Mengambil metadata file (permission, size, dll)
- `fs_readdir`: Menampilkan isi direktori
- `fs_open`: Membuka file sebelum operasi baca
- `fs_read`: Fungsi utama untuk membaca konten file


### 8. Fungsi main()

```
if (argc < 2) {
    fprintf(stderr, "Usage: %s <mountpoint_mnt> [FUSE opts]\n", argv[0]);
    return 1;
}
```
- Mengecek jumlah argumen saat menjalankan program
- Jika kurang dari 2 argumen (tidak menyertakan mount point (`mnt`), akan menampilkan pesan penggunaan:
  
    `Usage: ./program <mountpoint_mnt> [FUSE opts]`

```
realpath("anomali", src_dir);
snprintf(img_dir, sizeof(img_dir), "%s/image", src_dir);
mkdir(img_dir, 0755);
```
- `realpath("anomali", src_dir)`
    - Mengkonversi path relatif "anomali" ke path absolut
    - Menyimpan hasilnya di variabel global src_dir
    - Contoh: /home/user/anomali
- `snprintf(img_dir, ...)`:
    - Membentuk path untuk direktori gambar dengan format `{src_dir}/image`
    - Contoh: /home/user/anomali/image
 
- `mkdir(img_dir, 0755)`
    - Membuat direktori gambar dengan permission `755`
    - Jika direktori sudah ada, tidak akan membuat baru
```
umask(0);
return fuse_main(argc, argv, &ops, NULL);
```

- `umask(0)`: Memastikan/menyetel umask ke 0, memastikan permission file/direktori yang dibuat persis seperti yang ditentukan. Tanpa ini, permission asli bisa berbeda karena dipengaruhi oleh umask sistem.
- `fuse_main(argc, argv, &ops, NULL):`: Fungsi utama FUSE yang memulai filesystem virtual
    - `argc/argv`: Argument dari command line
    - `&ops:` Struct berisi pointer ke fungsi-fungsi filesystem (`fs_readdir`, `fs_open`, dll)
 
## Dokumentasi

![image](https://github.com/user-attachments/assets/4839e907-b0cf-4bca-9593-37358700091b)


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
