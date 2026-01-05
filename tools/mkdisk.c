/*
 * tools/mkdisk.c
 * Host tool to generate a GPT-partitioned disk image
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SECTOR_SIZE 512
#define DISK_SIZE_MB 128
#define DISK_SIZE_BYTES (DISK_SIZE_MB * 1024 * 1024)
#define NUM_SECTORS (DISK_SIZE_BYTES / SECTOR_SIZE)

/* GPT Constants */
#define GPT_SIGNATURE 0x5452415020494645ULL
#define GPT_REVISION 0x00010000

/* Simplified GUID structure */
struct guid {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  uint8_t data4[8];
};

/* Basic GUIDs */
struct guid TYPE_BOOT = {
    0x21686148,
    0x6449,
    0x6E6F,
    {0x74, 0x4E, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49}}; /* BIOS Boot */
struct guid TYPE_KERNEL = {
    0x0FC63DAF,
    0x8483,
    0x4772,
    {0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4}}; /* Linux Filesystem */
struct guid TYPE_DATA = {
    0x0FC63DAF,
    0x8483,
    0x4772,
    {0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4}}; /* Linux Filesystem */

struct gpt_header {
  uint64_t signature;
  uint32_t revision;
  uint32_t header_size;
  uint32_t header_crc32;
  uint32_t reserved1;
  uint64_t my_lba;
  uint64_t alternate_lba;
  uint64_t first_usable_lba;
  uint64_t last_usable_lba;
  struct guid disk_guid;
  uint64_t partition_entry_lba;
  uint32_t num_partition_entries;
  uint32_t partition_entry_size;
  uint32_t partition_entry_crc32;
} __attribute__((packed));

struct gpt_partition_entry {
  struct guid type_guid;
  struct guid unique_guid;
  uint64_t start_lba;
  uint64_t end_lba;
  uint64_t attributes;
  uint16_t partition_name[36];
} __attribute__((packed));

struct mbr_entry {
  uint8_t status;
  uint8_t chs_start[3];
  uint8_t type;
  uint8_t chs_end[3];
  uint32_t lba_start;
  uint32_t sectors;
} __attribute__((packed));

/* CRC32 Implementation */
uint32_t crc32(const void *data, size_t n_bytes) {
  uint32_t crc = 0xFFFFFFFF;
  const uint8_t *p = data;
  for (size_t i = 0; i < n_bytes; i++) {
    crc ^= p[i];
    for (int j = 0; j < 8; j++) {
      crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
  }
  return ~crc;
}

/* Ext4 Constants */
#define EXT4_SUPERBLOCK_OFFSET 1024
#define EXT4_MAGIC 0xEF53
#define EXT4_BLOCK_SIZE 4096
#define EXT4_SECTORS_PER_BLOCK (EXT4_BLOCK_SIZE / SECTOR_SIZE)
#define EXT4_INODE_SIZE 256
#define EXT4_ROOT_INO 2

/* Simplified Ext4 Structures */
struct ext4_superblock {
  uint32_t s_inodes_count;
  uint32_t s_blocks_count_lo;
  uint32_t s_r_blocks_count_lo;
  uint32_t s_free_blocks_count_lo;
  uint32_t s_free_inodes_count;
  uint32_t s_first_data_block;
  uint32_t s_log_block_size; /* 0=1KB, 1=2KB, 2=4KB */
  uint32_t s_log_cluster_size;
  uint32_t s_blocks_per_group;
  uint32_t s_clusters_per_group;
  uint32_t s_inodes_per_group;
  uint32_t s_mtime;
  uint32_t s_wtime;
  uint16_t s_mnt_count;
  uint16_t s_max_mnt_count;
  uint16_t s_magic;
  uint16_t s_state;
  uint16_t s_errors;
  uint16_t s_minor_rev_level;
  uint32_t s_lastcheck;
  uint32_t s_checkinterval;
  uint32_t s_creator_os;
  uint32_t s_rev_level;
  uint16_t s_def_resuid;
  uint16_t s_def_resgid;
  uint32_t s_first_ino;
  uint16_t s_inode_size;
  uint16_t s_block_group_nr;
  uint32_t s_feature_compat;
  uint32_t s_feature_incompat;
  uint32_t s_feature_ro_compat;
  uint8_t s_uuid[16];
  char s_volume_name[16];
  char s_last_mounted[64];
  uint32_t s_algorithm_usage_bitmap;
  uint8_t s_prealloc_blocks;
  uint8_t s_prealloc_dir_blocks;
  uint16_t s_reserved_gdt_blocks;
  uint8_t s_journal_uuid[16];
  uint32_t s_journal_inum;
  uint32_t s_journal_dev;
  uint32_t s_last_orphan;
  uint32_t s_hash_seed[4];
  uint8_t s_def_hash_version;
  uint8_t s_jnl_backup_type;
  uint16_t s_desc_size;
  uint32_t s_default_mount_opts;
  uint32_t s_first_meta_bg;
  uint32_t s_mkfs_time;
  uint32_t s_jnl_blocks[17];
  uint32_t s_blocks_count_hi;
  uint32_t s_r_blocks_count_hi;
  uint32_t s_free_blocks_count_hi;
  uint16_t s_min_extra_isize;
  uint16_t s_want_extra_isize;
  uint32_t s_flags;
  /* ... padding to 1024 bytes ... */
  uint8_t padding[1024 - 364];
} __attribute__((packed));

struct ext4_group_desc {
  uint32_t bg_block_bitmap_lo;
  uint32_t bg_inode_bitmap_lo;
  uint32_t bg_inode_table_lo;
  uint16_t bg_free_blocks_count_lo;
  uint16_t bg_free_inodes_count_lo;
  uint16_t bg_used_dirs_count_lo;
  uint16_t bg_flags;
  /* ... ignoring modern features for now ... */
  uint8_t padding[14];
} __attribute__((packed));

struct ext4_inode {
  uint16_t i_mode;
  uint16_t i_uid;
  uint32_t i_size_lo;
  uint32_t i_atime;
  uint32_t i_ctime;
  uint32_t i_mtime;
  uint32_t i_dtime;
  uint16_t i_gid;
  uint16_t i_links_count;
  uint32_t i_blocks_lo;
  uint32_t i_flags;
  uint32_t i_osd1;
  uint32_t i_block[15]; /* Pointers */
  /* ... truncated ... */
  uint8_t
      padding[256 - 102]; /* Pad to exactly 256 bytes. Corrected padding size */
} __attribute__((packed));

struct ext4_dir_entry {
  uint32_t inode;
  uint16_t rec_len;
  uint8_t name_len;
  uint8_t file_type;
  char name[];
} __attribute__((packed));

/* Global Allocator State */
static uint32_t next_free_block = 11; /* Start after metadata (blocks 0-10) */

/* Helper: Write a file to the disk image */
void write_file_to_inode(FILE *f, uint64_t start_offset, uint32_t ino,
                         const char *src_path) {
  uint64_t inode_offset =
      start_offset + 4 * EXT4_BLOCK_SIZE + (ino - 1) * EXT4_INODE_SIZE;

  struct ext4_inode file_inode = {0};
  file_inode.i_mode = 0x81C0; /* File | 644 */
  file_inode.i_links_count = 1;

  FILE *src = fopen(src_path, "rb");
  if (!src) {
    /* Fallback placeholder */
    printf("Warning: %s not found. Using placeholder.\n", src_path);
    file_inode.i_size_lo = 16;
    file_inode.i_blocks_lo = 8;
    file_inode.i_block[0] = next_free_block; // Use 1 block

    /* Write Inode */
    fseek(f, inode_offset, SEEK_SET);
    fwrite(&file_inode, 1, sizeof(file_inode), f);

    /* Write Placeholder Data */
    fseek(f, start_offset + next_free_block * EXT4_BLOCK_SIZE, SEEK_SET);
    fwrite("PLACEHOLDER_FILE", 1, 16, f);

    next_free_block++;
    return;
  }

  /* Real File Write */
  fseek(src, 0, SEEK_END);
  long src_size = ftell(src);
  rewind(src);

  uint8_t *buf = malloc(src_size);
  if (!buf) {
    perror("malloc");
    fclose(src);
    exit(1);
  }
  fread(buf, 1, src_size, src);
  fclose(src);

  file_inode.i_size_lo = src_size;
  int data_blocks = (src_size + EXT4_BLOCK_SIZE - 1) / EXT4_BLOCK_SIZE;
  int total_blocks = data_blocks;

  uint32_t indir_block = 0;
  if (data_blocks > 12) {
    /* Need 1 indirect block */
    indir_block = next_free_block + data_blocks; /* Place at end of data */
    total_blocks++; /* Account for indirect block overhead */
  }

  /* 512-byte sectors count for i_blocks */
  file_inode.i_blocks_lo = total_blocks * (EXT4_BLOCK_SIZE / 512);

  /* Assign Direct Blocks */
  for (int i = 0; i < data_blocks && i < 12; i++) {
    file_inode.i_block[i] = next_free_block + i;
  }

  /* Assign Indirect Block */
  if (data_blocks > 12) {
    file_inode.i_block[12] = indir_block;

    /* Prep Indirect Data */
    uint32_t *indirect_buf = calloc(1, EXT4_BLOCK_SIZE);
    for (int i = 12; i < data_blocks; i++) {
      indirect_buf[i - 12] = next_free_block + i;
    }

    /* Write Indirect Block */
    fseek(f, start_offset + indir_block * EXT4_BLOCK_SIZE, SEEK_SET);
    fwrite(indirect_buf, 1, EXT4_BLOCK_SIZE, f);
    free(indirect_buf);
  }

  /* Write Inode */
  fseek(f, inode_offset, SEEK_SET);
  fwrite(&file_inode, 1, sizeof(file_inode), f);

  /* Write Data Content */
  fseek(f, start_offset + next_free_block * EXT4_BLOCK_SIZE, SEEK_SET);
  fwrite(buf, 1, src_size, f);

  free(buf);

  printf("Ext4: Added %s (Ino %d, %ld bytes, %d blocks)\n", src_path, ino,
         src_size, total_blocks);

  /* Advance Allocator */
  next_free_block += total_blocks;
}

void write_ext4_partition(FILE *f, uint64_t start_lba, uint64_t size_sectors) {
  uint64_t start_offset = start_lba * SECTOR_SIZE;
  uint64_t size_bytes = size_sectors * SECTOR_SIZE;
  uint32_t num_blocks = size_bytes / EXT4_BLOCK_SIZE;

  printf("Ext4: Formatting partition at LBA %llu (Size: %d MB)\n",
         (unsigned long long)start_lba, (int)(size_bytes >> 20));

  /* Seek to partition start */
  fseek(f, start_offset + EXT4_SUPERBLOCK_OFFSET, SEEK_SET);

  /* 1. Superblock */
  struct ext4_superblock sb = {0};
  sb.s_inodes_count = 1024;
  sb.s_blocks_count_lo = num_blocks;
  sb.s_free_blocks_count_lo = num_blocks - 100; /* Estimate */
  sb.s_free_inodes_count = 1010;
  sb.s_first_data_block = 0; /* 4KB blocks */
  sb.s_log_block_size = 2;   /* 4KB = 1024 << 2 */
  sb.s_blocks_per_group = 8192;
  sb.s_clusters_per_group = 8192;
  sb.s_inodes_per_group = 1024;
  sb.s_magic = EXT4_MAGIC;
  sb.s_state = 1;
  sb.s_rev_level = 1;
  sb.s_first_ino = 11;
  sb.s_inode_size = EXT4_INODE_SIZE;

  fwrite(&sb, 1, 1024, f);

  /* 2. Group Descriptors (Block 1) */
  /* We just have 1 group for simplicity (< 32MB fits in 1 group) */
  fseek(f, start_offset + EXT4_BLOCK_SIZE, SEEK_SET); // Block 1
  struct ext4_group_desc bg = {0};
  bg.bg_block_bitmap_lo = 2;
  bg.bg_inode_bitmap_lo = 3;
  bg.bg_inode_table_lo = 4;
  bg.bg_free_blocks_count_lo = 100;
  bg.bg_free_inodes_count_lo = 100;
  bg.bg_used_dirs_count_lo = 2;
  fwrite(&bg, 1, sizeof(bg), f);

  /* 3. Block Bitmap (Block 2) */
  fseek(f, start_offset + 2 * EXT4_BLOCK_SIZE, SEEK_SET);
  uint8_t bmap[EXT4_BLOCK_SIZE] = {0};
  bmap[0] = 0xFF; /* First 8 blocks used (metadata) */
  /* Better block bitmap management needed for real usage */
  fwrite(bmap, 1, EXT4_BLOCK_SIZE, f);

  /* 4. Inode Bitmap (Block 3) */
  fseek(f, start_offset + 3 * EXT4_BLOCK_SIZE, SEEK_SET);
  uint8_t imap[EXT4_BLOCK_SIZE] = {0};
  imap[0] = 0x03; // Inode 1 (Reserved), Inode 2 (Root) used.
  /* Mark Inode 11 and 12 as used (Bits 2 and 3 of Byte 1) */
  imap[1] = 0x0C; // (1<<2) | (1<<3) = 4 | 8 = 12 = 0x0C
  fwrite(imap, 1, EXT4_BLOCK_SIZE, f);

  /* 5. Inode Table (Block 4) */
  /* Inode 2: Root Directory */
  fseek(f, start_offset + 4 * EXT4_BLOCK_SIZE + (2 - 1) * EXT4_INODE_SIZE,
        SEEK_SET);
  struct ext4_inode root = {0};
  root.i_mode = 0x41ED; /* Directory | 755 */
  root.i_links_count = 2;
  root.i_size_lo = 4096;
  root.i_blocks_lo = 8; /* 512-byte sectors = 4KB */
  root.i_block[0] = 10; /* Data at Block 10 */
  fwrite(&root, 1, sizeof(root), f);

  /* 6. Root Directory Data (Block 10) */
  fseek(f, start_offset + 10 * EXT4_BLOCK_SIZE, SEEK_SET);
  uint8_t dir_blk[EXT4_BLOCK_SIZE] = {0};
  int off = 0;

  /* "." entry */
  struct ext4_dir_entry *de = (struct ext4_dir_entry *)&dir_blk[off];
  de->inode = 2;
  de->rec_len = 12;
  de->name_len = 1;
  de->file_type = 2; /* Dir */
  memcpy(de->name, ".", 1);
  off += de->rec_len;

  /* ".." entry */
  de = (struct ext4_dir_entry *)&dir_blk[off];
  de->inode = 2;
  de->rec_len = 12;
  de->name_len = 2;
  de->file_type = 2;
  memcpy(de->name, "..", 2);
  off += de->rec_len;

  /* "init" entry */
  de = (struct ext4_dir_entry *)&dir_blk[off];
  de->inode = 11;
  de->rec_len = 16; /* 4N-aligned */
  de->name_len = 4;
  de->file_type = 1; /* File */
  memcpy(de->name, "init", 4);
  off += de->rec_len;

  /* "counter" entry */
  de = (struct ext4_dir_entry *)&dir_blk[off];
  de->inode = 12;
  de->rec_len = 20;
  de->name_len = 7;
  de->file_type = 1;
  memcpy(de->name, "counter", 7);
  off += de->rec_len;

  /* "shell" entry */
  de = (struct ext4_dir_entry *)&dir_blk[off];
  de->inode = 13;
  de->rec_len = EXT4_BLOCK_SIZE - off; /* Rest of block */
  de->name_len = 5;
  de->file_type = 1;
  memcpy(de->name, "shell", 5);

  fwrite(dir_blk, 1, EXT4_BLOCK_SIZE, f);

  /* 7. Write Files */
  /* This updates Inode Table and writes Data Blocks, updating next_free_block
   */
  write_file_to_inode(f, start_offset, 11, "build/init.elf");
  write_file_to_inode(f, start_offset, 12, "build/counter.elf");
  write_file_to_inode(f, start_offset, 13, "build/shell.elf");
  write_file_to_inode(f, start_offset, 14, "build/demo3d.elf");

  printf("Ext4: Filesystem created.\n");
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "wb+");
  if (!f) {
    perror("fopen");
    return 1;
  }

  /* 1. Zero out disk */
  printf("Creating %dMB disk image...\n", DISK_SIZE_MB);
  // Writing 128MB of zeros is slow, let's seek and write last byte
  fseek(f, DISK_SIZE_BYTES - 1, SEEK_SET);
  fputc(0, f);
  rewind(f);

  /* 2. Protective MBR (LBA 0) */
  uint8_t mbr[SECTOR_SIZE] = {0};
  mbr[510] = 0x55;
  mbr[511] = 0xAA;
  struct mbr_entry *entry = (struct mbr_entry *)&mbr[446];
  entry->status = 0x00;
  entry->type = 0xEE; /* GPT Protective */
  entry->lba_start = 1;
  entry->sectors = NUM_SECTORS - 1;
  fwrite(mbr, 1, SECTOR_SIZE, f);

  /* 3. Prepare Partition Entries */
  /* 128 entries of 128 bytes = 16 sectors (16 * 512 = 8192 bytes) */
  uint8_t *entries = calloc(128, 128);
  struct gpt_partition_entry *e = (struct gpt_partition_entry *)entries;

  /* Partition 1: Boot (1MB) LBA 34 to 2081 */
  e[0].type_guid = TYPE_BOOT;
  e[0].start_lba = 34;
  e[0].end_lba = 2081; /* 1MB approx */

  /* Partition 2: Kernel (16MB) LBA 2082 to 34849 */
  e[1].type_guid = TYPE_KERNEL;
  e[1].start_lba = 2082;
  e[1].end_lba = 34849; /* 16MB approx */

  /* Partition 3: Userland (Rest) */
  e[2].type_guid = TYPE_DATA;
  e[2].start_lba = 34850;
  e[2].end_lba = NUM_SECTORS - 34; /* Leave room for secondary GPT */

  uint32_t entries_crc = crc32(entries, 128 * 128);

  /* 4. Prepare GPT Header (LBA 1) */
  struct gpt_header h = {0};
  h.signature = GPT_SIGNATURE;
  h.revision = GPT_REVISION;
  h.header_size = 92;
  h.my_lba = 1;
  h.alternate_lba = NUM_SECTORS - 1;
  h.first_usable_lba = 34;
  h.last_usable_lba = NUM_SECTORS - 34;
  h.partition_entry_lba = 2;
  h.num_partition_entries = 128;
  h.partition_entry_size = 128;
  h.partition_entry_crc32 = entries_crc;
  h.header_crc32 = 0; /* Clear for calc */
  h.header_crc32 = crc32(&h, 92);

  /* Write GPT Header (LBA 1) */
  fwrite(&h, 1, sizeof(h), f);
  /* Pad to sector size */
  uint8_t pad[SECTOR_SIZE - sizeof(h)];
  memset(pad, 0, sizeof(pad));
  fwrite(pad, 1, sizeof(pad), f);

  /* Write Partition Entries (LBA 2 onwards) */
  fwrite(entries, 1, 128 * 128, f);

  /* TODO: Write Secondary GPT (Header + Entries) at end of disk */
  /* For now, primary GPT is enough for our kernel parser */

  printf("Disk image created successfully: %s\n", argv[1]);

  /* Format Partition 3 as Ext4 */
  write_ext4_partition(f, e[2].start_lba, e[2].end_lba - e[2].start_lba + 1);

  free(entries);
  fclose(f);
  return 0;
}
