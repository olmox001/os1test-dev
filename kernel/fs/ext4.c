/*
 * kernel/fs/ext4.c
 * Simplified Read-Only Ext4 Driver (With Random Access)
 */
#include <drivers/virtio_blk.h>
#include <kernel/buffer.h>
#include <kernel/ext4.h>
#include <kernel/gpt.h>
#include <kernel/printk.h>
#include <kernel/string.h>

/* Global State */
static uint64_t part_start_lba;
static struct ext4_superblock sb;
static struct ext4_group_desc bg;

/*
 * Initialize Ext4
 */
void ext4_init(void) {
  /* 1. Find Userland Partition (Index 2) */
  struct partition *p = gpt_get_partition(2);
  if (!p) {
    pr_err("Ext4: Partition 2 not found!\n");
    return;
  }
  part_start_lba = p->start_lba;
  pr_info("Ext4: Found partition at LBA %ld\n", part_start_lba);

  /* 2. Read Superblock (Offset 1024) - LBA + 2 */
  uint8_t buf[4096];
  if (virtio_blk_read(buf, part_start_lba + 2, 2) != 0) {
    pr_err("Ext4: Failed to read Superblock\n");
    return;
  }
  memcpy(&sb, buf, sizeof(sb));

  if (sb.s_magic != EXT4_MAGIC) {
    pr_err("Ext4: Invalid Magic: %x\n", sb.s_magic);
    return;
  }

  pr_info("Ext4: Mounted. Vol=%s, Inodes=%d\n", sb.s_volume_name,
          sb.s_inodes_count);

  /* 3. Read Block Group Descriptor 0 (Block 1) */
  /* GDT starts at Block 1 (Bytes 4096). 8 Sectors from start. */
  if (virtio_blk_read(buf, part_start_lba + 8, 1) != 0) {
    pr_err("Ext4: Failed to read GDT\n");
    return;
  }
  memcpy(&bg, buf, sizeof(bg));

  pr_info("Ext4: Group 0: Inode Table at Block %d\n", bg.bg_inode_table_lo);
}

/*
 * Helper: Read an Inode Structure from Table
 */
static int get_inode_struct(uint32_t ino, struct ext4_inode *inode_out) {
  /* Inode Table Block */
  uint64_t table_blk = bg.bg_inode_table_lo;
  uint64_t table_byte_offset = table_blk * 4096;
  uint64_t inode_offset = table_byte_offset + (ino - 1) * EXT4_INODE_SIZE;

  uint64_t sector = part_start_lba + (inode_offset / 512);
  uint32_t sector_off = inode_offset % 512;

  uint8_t buf[512];
  if (virtio_blk_read(buf, sector, 1) != 0)
    return -1;

  memcpy(inode_out, buf + sector_off, sizeof(struct ext4_inode));
  return 0;
}

/*
 * Public API: Read Data from Inode (Random Access)
 * Supports Direct Blocks (0-11). Does NOT yet support Indirect Blocks.
 */
int ext4_read_inode(uint32_t ino, uint32_t offset, uint8_t *buf,
                    uint32_t size) {
  struct ext4_inode inode;
  if (get_inode_struct(ino, &inode) != 0)
    return -1;

  uint32_t file_size = inode.i_size_lo;
  if (offset >= file_size)
    return 0;
  if (offset + size > file_size)
    size = file_size - offset;

  uint32_t bytes_read = 0;
  uint32_t current_offset = offset;

  while (bytes_read < size) {
    /* Calculate Block Index in File */
    uint32_t block_idx = current_offset / 4096;
    uint32_t block_off = current_offset % 4096;
    uint32_t to_copy = 4096 - block_off;
    if (to_copy > (size - bytes_read))
      to_copy = size - bytes_read;

    /* Get Physical Block Number */
    uint32_t phys_block = 0;

    if (block_idx < 12) {
      /* Direct Block */
      phys_block = inode.i_block[block_idx];
    } else {
      /* Indirect Block (Index 12 points to block of pointers) */
      uint32_t indirect_blk_num = inode.i_block[12];

      if (indirect_blk_num == 0) {
        pr_err("Ext4: Indirect block not allocated!\n");
        return -1;
      }

      /* Read Indirect Block */
      /* Note: For performance, we should cache this. For now, read every time.
       */
      uint8_t indirect_buf[4096];
      /* Indirect block is 1 sector? No, 1 Block (4096 bytes / 8 sectors) */
      uint64_t sector_indir = part_start_lba + (indirect_blk_num * 8);
      if (virtio_blk_read(indirect_buf, sector_indir, 8) != 0) {
        pr_err("Ext4: Failed to read Indirect Block at %d\n", indirect_blk_num);
        return -1;
      }

      uint32_t *pointers = (uint32_t *)indirect_buf;
      uint32_t indirect_idx = block_idx - 12;

      if (indirect_idx >= 1024) { /* 4096 / 4 = 1024 pointers */
        pr_err("Ext4: Double Indirect Blocks not supported (Index %d)\n",
               block_idx);
        return -1;
      }

      phys_block = pointers[indirect_idx];
    }

    /* Read the Block */
    uint8_t block_buf[4096];
    if (phys_block == 0) {
      /* Sparse hole */
      memset(block_buf, 0, 4096);
    } else {
      uint64_t sector = part_start_lba + (phys_block * 8);
      if (virtio_blk_read(block_buf, sector, 8) != 0) {
        return -1;
      }
    }

    /* Copy Data */
    memcpy(buf + bytes_read, block_buf + block_off, to_copy);

    bytes_read += to_copy;
    current_offset += to_copy;
  }

  return bytes_read;
}

/*
 * Public API: Find Inode by Path (Root Directory Only for now)
 */
int ext4_find_inode(const char *path, uint32_t *ino_out) {
  /* Skip leading slash */
  const char *target = path;
  if (target[0] == '/')
    target++;

  /* 1. Read Root Inode (2) to get Data Block */
  struct ext4_inode root;
  if (get_inode_struct(2, &root) != 0)
    return -1;

  /* 2. Read Root Directory Data (Block 0) */
  /* Assuming Root Dir fits in 1 Block for now */
  uint8_t dir_buf[4096];
  if (ext4_read_inode(2, 0, dir_buf, 4096) <= 0)
    return -1;

  /* 3. Scan Entries */
  struct ext4_dir_entry *de = (struct ext4_dir_entry *)dir_buf;
  uint32_t offset = 0;

  while (offset < 4096) {
    if (de->inode == 0)
      break; /* End of entries */

    /* Compare Name */
    if (de->name_len == strlen(target)) {
      if (memcmp(de->name, target, de->name_len) == 0) {
        *ino_out = de->inode;
        return 0; /* Found */
      }
    }

    offset += de->rec_len;
    de = (struct ext4_dir_entry *)((uint8_t *)de + de->rec_len);
  }

  return -1; /* Not Found */
}

/*
 * Legacy Wrapper
 */
int ext4_read_file(const char *path, uint8_t *buf, uint32_t size) {
  uint32_t ino;
  if (ext4_find_inode(path, &ino) != 0) {
    pr_err("Ext4: File not found: %s\n", path);
    return -1;
  }
  return ext4_read_inode(ino, 0, buf, size);
}
