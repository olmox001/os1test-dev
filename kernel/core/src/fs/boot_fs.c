#include <libkernel/types.h>
#include <core/printk.h>
#include <libkernel/string.h>
#include <core/kmalloc.h>
#include <core/boot_fs.h>
#include <hal/drivers/virtio_blk.h>

/* Simplified GPT / Ext4 for boot only */
#define SECTOR_SIZE 512
#define EXT4_MAGIC 0xEF53

struct gpt_header {
    uint64_t signature;
    uint32_t revision;
    uint32_t size;
    uint32_t crc32;
    uint32_t reserved;
    uint64_t my_lba;
    uint64_t alt_lba;
    uint64_t first_usable;
    uint64_t last_usable;
    uint8_t guid[16];
    uint64_t part_entry_lba;
    uint32_t num_parts;
    uint32_t part_entry_size;
} __attribute__((packed));

struct gpt_entry {
    uint8_t type_guid[16];
    uint8_t part_guid[16];
    uint64_t start_lba;
    uint64_t end_lba;
    uint64_t attributes;
    uint16_t name[36];
} __attribute__((packed));

struct mbr_partition_entry {
    uint8_t status;
    uint8_t chs_start[3];
    uint8_t type;
    uint8_t chs_end[3];
    uint32_t start_lba;
    uint32_t sector_count;
} __attribute__((packed));

struct ext4_superblock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count_lo;
    uint32_t s_r_blocks_count_lo;
    uint32_t s_free_blocks_count_lo;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
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
} __attribute__((packed));

struct ext4_group_desc {
    uint32_t bg_block_bitmap_lo;
    uint32_t bg_inode_bitmap_lo;
    uint32_t bg_inode_table_lo;
    /* ... */
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
    uint32_t i_block[15];
    /* ... */
} __attribute__((packed));

struct ext4_dir_entry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[];
} __attribute__((packed));

static uint64_t data_part_lba = 0;
static uint32_t inode_size = 256;
static uint32_t block_size = 4096;

/* Helper to read blocks from partition */
static int read_blocks(uint64_t block, void *buf, uint32_t count) {
    uint64_t sector = data_part_lba + (block * (block_size / SECTOR_SIZE));
    return virtio_blk_read(buf, sector, count * (block_size / SECTOR_SIZE));
}

int boot_fs_init(void) {
    uint8_t *buf = kmalloc(SECTOR_SIZE);
    if (virtio_blk_read(buf, 1, 1) != 0) {
        pr_err("%s", "BootFS: Failed to read GPT header\n");
        return -1;
    }

    struct gpt_header *h = (struct gpt_header *)buf;
    if (h->signature != 0x5452415020494645ULL) {
        pr_warn("BootFS: Invalid GPT signature, attempting MBR fallback...\n");
        if (virtio_blk_read(buf, 0, 1) != 0) {
            pr_err("BootFS: Failed to read MBR sector\n");
            return -1;
        }
        if (buf[510] != 0x55 || buf[511] != 0xAA) {
            pr_err("BootFS: Invalid MBR boot signature\n");
            return -1;
        }
        struct mbr_partition_entry *e = (struct mbr_partition_entry *)(buf + 446);
        bool found = false;
        for (int i = 0; i < 4; i++) {
            if (e[i].type == 0x83) { // Linux Native partition
                data_part_lba = e[i].start_lba;
                pr_info("BootFS: Found MBR Ext4 partition %d at LBA %ld\n", i + 1, data_part_lba);
                found = true;
                break;
            }
        }
        if (!found) {
            pr_err("BootFS: No Ext4 partition (0x83) found in MBR\n");
            return -1;
        }
    } else {
        /* Read partition entries (simplified: check first 4) */
        uint64_t entry_lba = h->part_entry_lba;
        if (virtio_blk_read(buf, entry_lba, 1) != 0) return -1;

        struct gpt_entry *e = (struct gpt_entry *)buf;
        /* We expect the 3rd partition (index 2) to be DATA */
        data_part_lba = e[2].start_lba;
        pr_info("BootFS: Found GPT Ext4 partition at LBA %ld\n", data_part_lba);
    }

    /* Read Ext4 Superblock */
    uint8_t *sb_buf = kmalloc(1024);
    if (virtio_blk_read(sb_buf, data_part_lba + 2, 2) != 0) return -1;
    struct ext4_superblock *sb = (struct ext4_superblock *)sb_buf;
    
    if (sb->s_magic != EXT4_MAGIC) {
        pr_err("BootFS: Invalid Ext4 magic (0x%x)\n", sb->s_magic);
        return -1;
    }
    
    inode_size = sb->s_inode_size;
    pr_info("BootFS: Ext4 initialized (Inode Size: %d)\n", inode_size);
    
    kfree(buf);
    kfree(sb_buf);
    return 0;
}

static struct ext4_inode *get_inode(uint32_t ino) {
    /* Simplified: assume everything is in group 0 */
    uint8_t *bg_buf = kmalloc(4096);
    if (read_blocks(1, bg_buf, 1) != 0) return NULL;
    struct ext4_group_desc *bg = (struct ext4_group_desc *)bg_buf;
    
    uint32_t table_block = bg->bg_inode_table_lo;
    uint32_t block = table_block + ((ino - 1) * inode_size) / block_size;
    uint32_t offset = ((ino - 1) * inode_size) % block_size;
    
    uint8_t *table_buf = kmalloc(block_size);
    if (read_blocks(block, table_buf, 1) != 0) return NULL;
    
    struct ext4_inode *inode = kmalloc(sizeof(struct ext4_inode));
    memcpy(inode, table_buf + offset, sizeof(struct ext4_inode));
    
    kfree(bg_buf);
    kfree(table_buf);
    return inode;
}

uint32_t boot_fs_find_inode(const char *path) {
    if (strcmp(path, "/") == 0) return 2;
    
    uint32_t current_ino = 2; // Root
    char p[128];
    strncpy(p, path, 127);
    
    char *token = strtok(p, "/");
    while (token) {
        struct ext4_inode *inode = get_inode(current_ino);
        if (!inode) return 0;
        
        /* Read first data block of directory */
        uint8_t *dir_buf = kmalloc(block_size);
        if (read_blocks(inode->i_block[0], dir_buf, 1) != 0) return 0;
        
        struct ext4_dir_entry *de = (struct ext4_dir_entry *)dir_buf;
        uint32_t found_ino = 0;
        uint32_t offset = 0;
        
        while (offset < inode->i_size_lo && de->rec_len > 0) {
            if (de->name_len == strlen(token) && strncmp(de->name, token, de->name_len) == 0) {
                found_ino = de->inode;
                break;
            }
            offset += de->rec_len;
            de = (struct ext4_dir_entry *)(dir_buf + offset);
        }
        
        kfree(dir_buf);
        kfree(inode);
        
        if (!found_ino) return 0;
        current_ino = found_ino;
        token = strtok(NULL, "/");
    }
    
    return current_ino;
}

int boot_fs_read_inode(uint32_t ino, uint64_t offset, uint8_t *buf, uint32_t size) {
    struct ext4_inode *inode = get_inode(ino);
    if (!inode) return -1;
    
    uint32_t start_block = offset / block_size;
    uint32_t end_block = (offset + size + block_size - 1) / block_size;
    uint32_t bytes_read = 0;
    
    uint8_t *tmp_buf = kmalloc(block_size);
    uint8_t *indirect_buf = kmalloc(block_size);
    if (!tmp_buf || !indirect_buf) {
        if (tmp_buf) kfree(tmp_buf);
        if (indirect_buf) kfree(indirect_buf);
        kfree(inode);
        return -1;
    }
    
    for (uint32_t b = start_block; b < end_block; b++) {
        uint32_t phys_block = 0;
        
        if (b < 12) {
            phys_block = inode->i_block[b];
        } else if (b < 12 + 1024) {
            uint32_t indirect_blk_num = inode->i_block[12];
            if (indirect_blk_num != 0) {
                if (read_blocks(indirect_blk_num, indirect_buf, 1) == 0) {
                    phys_block = ((uint32_t *)indirect_buf)[b - 12];
                }
            }
        } else {
            uint32_t d_idx = b - 12 - 1024;
            uint32_t master_idx = d_idx / 1024;
            uint32_t sub_idx = d_idx % 1024;
            
            uint32_t double_indir_blk = inode->i_block[13];
            if (double_indir_blk != 0 && master_idx < 1024) {
                if (read_blocks(double_indir_blk, indirect_buf, 1) == 0) {
                    uint32_t sub_indir_blk = ((uint32_t *)indirect_buf)[master_idx];
                    if (sub_indir_blk != 0) {
                        if (read_blocks(sub_indir_blk, tmp_buf, 1) == 0) {
                            phys_block = ((uint32_t *)tmp_buf)[sub_idx];
                        }
                    }
                }
            }
        }
        
        if (phys_block == 0) {
            memset(tmp_buf, 0, block_size);
        } else {
            if (read_blocks(phys_block, tmp_buf, 1) != 0) {
                memset(tmp_buf, 0, block_size);
            }
        }
        
        uint32_t block_off = (b == start_block) ? (offset % block_size) : 0;
        uint32_t to_copy = block_size - block_off;
        if (bytes_read + to_copy > size) to_copy = size - bytes_read;
        
        memcpy(buf + bytes_read, tmp_buf + block_off, to_copy);
        bytes_read += to_copy;
    }
    
    kfree(tmp_buf);
    kfree(indirect_buf);
    kfree(inode);
    return bytes_read;
}

/* List directory entries for path into buf (newline-separated names).
 * Returns number of bytes written or -1 on error. */
int boot_fs_list_dir(const char *path, char *buf, size_t size) {
    if (!path || !buf || size == 0) return -1;

    uint32_t ino = boot_fs_find_inode(path);
    if (!ino) return -1;

    struct ext4_inode *inode = get_inode(ino);
    if (!inode) return -1;

    /* Read first directory block (simplified: only block 0) */
    uint8_t *dir_buf = kmalloc(block_size);
    if (!dir_buf) { kfree(inode); return -1; }
    if (read_blocks(inode->i_block[0], dir_buf, 1) != 0) {
        kfree(dir_buf); kfree(inode); return -1;
    }

    size_t off     = 0;
    uint32_t pos   = 0;
    uint32_t dsize = inode->i_size_lo < block_size ? inode->i_size_lo : block_size;

    while (pos < dsize) {
        struct ext4_dir_entry *de = (struct ext4_dir_entry *)(dir_buf + pos);
        if (de->rec_len == 0) break;

        if (de->inode != 0 && de->name_len > 0) {
            /* Skip . and .. */
            if (!(de->name_len == 1 && de->name[0] == '.') &&
                !(de->name_len == 2 && de->name[0] == '.' && de->name[1] == '.')) {
                if (off + de->name_len + 2 > size) break;
                memcpy(buf + off, de->name, de->name_len);
                off += de->name_len;
                buf[off++] = '\n';
            }
        }
        pos += de->rec_len;
    }

    buf[off] = '\0';
    kfree(dir_buf);
    kfree(inode);
    return (int)off;
}
