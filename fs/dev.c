#include "fs.h"
#include <winix/list.h>

struct device _root_dev;
struct list_head devices_list;
struct superblock root_sb;
static struct file_system root_fs;
struct device_operations dops;
struct filp_operations ops;
dev_t devid = 0;

char* DEVICE_NAME = "sda";
char* FS_TYPE = "wfs";

disk_word_t* _disk;
size_t _disk_size;


struct superblock* get_sb(struct device* id){
    if(id == root_fs.device)
        return root_fs.sb;
    return NULL;
}

void sync_sb(struct superblock* sb){

}


// void report_sb(struct superblock* sb){
//     disk_word_t curr = 0;
//     printf("\nsuper block 0 - 0x%08x\n", curr);
//     curr += sb->s_superblock_size;
//     printf("block map 0x%08x - 0x%08x\n",curr, curr+ sb->s_blockmap_size );
//     curr += sb->s_blockmap_size;
//     printf("inode map 0%08x - 0x%08x\n",curr, curr+sb->s_inodemap_size );
//     curr += sb->s_inodemap_size;
//     printf("inode table 0x%08x - 0x%08x\n",curr, curr+sb->s_inode_table_size );
//     curr += sb->s_inode_table_size;
//     printf("data block 0x%08x - 0x%p\n Number of free blocks %d\n",curr, _disk + _disk_size - curr, sb->s_free_blocks );

//     printf("\nNum of blocks in use %d\n", sb->s_block_inuse);
//     printf("First free block num: %d\n", sb->s_nblock);
//     printf("Block size %d\n inode size %d\n", sb->s_block_size, sb->s_inode_size);
//     printf("inode per block %d\n", sb->s_inode_per_block);
// }

int blk_dev_io_read(disk_word_t *buf, off_t off, size_t len){
    disk_word_t *ptr;
    int count = len;
    if(off + len > _disk_size)
        return 0;
    ptr = _disk + off;
    while(count-- > 0){
        *buf++ = *ptr++;
    }
    return len;
}

int blk_dev_io_write(disk_word_t *buf, off_t off, size_t len){
    disk_word_t *ptr;
    int count = len;
    if(off + len > _disk_size)
        return 0;
    ptr = _disk + off;
    while(count-- > 0){
        *ptr++ = *buf++;
    }
    return len;
}

int blk_dev_init(){
    _disk_size = DISK_SIZE;
    _disk = DISK_RAW;

    memcpy(root_fs.sb, _disk, sizeof(struct superblock));
    root_fs.sb->s_iroot = NULL;
    return 0;
}

int blk_dev_release(){
    return 0;
}

int root_fs_read (struct filp *filp, char *data, size_t count, off_t offset){
    int ret = 0, r, j;
    unsigned int len;
    off_t off, ino_size, end_in_block;
    unsigned int curr_fp_index, fp_limit;
    block_t bnr;
    inode_t *ino = NULL;
    struct block_buffer* buffer;

    curr_fp_index = offset / BLOCK_SIZE;
    off = offset % BLOCK_SIZE;
    fp_limit = (filp->filp_pos + count ) / BLOCK_SIZE;
    ino = filp->filp_ino;
    ino_size = ino->i_size;

    for( ; curr_fp_index <= fp_limit; curr_fp_index++){
        len = ((BLOCK_SIZE - off) > count) ? count : BLOCK_SIZE - off;
        bnr = ino->i_zone[curr_fp_index];
        if(bnr == 0){
            break;
        }

        r = 0;
        buffer = get_block_buffer(bnr, filp->filp_dev);
        end_in_block = (off + len) < ino->i_size ? (off + len) : ino->i_size;
        for (j = off; j < end_in_block; j++) {
            *data++ = buffer->block[j];
            r++;
        }
        KDEBUG(("file read for block %d, off %d len %d\n", bnr, off, r));
        put_block_buffer(buffer);
        if (r == 0)
            break;

        count -= len;
        ret += r;
        filp->filp_pos += r;
        off = 0;
    }
    return ret;
}

int root_fs_write (struct filp *filp, char *data, size_t count, off_t offset){
    int r, j, ret = 0;
    unsigned int len;
    off_t off;
    unsigned int curr_fp_index, fp_limit;
    block_t bnr;
    inode_t *ino = NULL;
    struct block_buffer* buffer;

    curr_fp_index = offset / BLOCK_SIZE;
    off = offset % BLOCK_SIZE;
    fp_limit = (filp->filp_pos + count ) / BLOCK_SIZE;
    ino = filp->filp_ino;

    for( ; curr_fp_index <= fp_limit; curr_fp_index++){
        len = ((BLOCK_SIZE - off) > count) ? count : BLOCK_SIZE - off;
        bnr = ino->i_zone[curr_fp_index];
        if(bnr == 0){
            if(curr_fp_index < NR_TZONES && (bnr = alloc_block(ino, ino->i_dev)) > 0){
                ino->i_zone[curr_fp_index] = bnr;
            }else{
                return ENOSPC;
            }
        }
        buffer = get_block_buffer(bnr, filp->filp_dev);

        r = 0;
        for (j = off; j< off + len && j < BLOCK_SIZE; j++) {
            buffer->block[j] = *data++;
            r++;
        }
        KDEBUG(("file write for block %d, off %d len %d\n", curr_fp_index, off, r));
        put_block_buffer_dirt(buffer);
        /* Read or write 'chunk' bytes. */
        if (r == 0)
            break;

        count -= len;
        ret += r;
        filp->filp_pos += r;
        filp->filp_ino->i_size += r;
        off = 0;
    }
    return ret;
}

int root_fs_open (struct inode* ino, struct filp *file){
    return 0;
}

int root_fs_close (struct inode* ino, struct filp *file){
    return 0;
}

void init_dev(){
    INIT_LIST_HEAD(&devices_list);
}

int register_device(struct device* dev, const char* name, dev_t id, mode_t type){
    dev->init_name = name;
    dev->dev_id = id;
    dev->device_type = type;
    list_add(&dev->list,&devices_list);
}

struct device* get_dev(dev_t dev){
    struct device* ret;
    list_for_each_entry(struct device, ret, &devices_list, list){
        if(ret->dev_id == dev){
            return ret;
        }
    }
    return NULL;
}

void init_root_fs(){
    struct superblock test;
    struct inode* ino;
    struct device *dev = &_root_dev;
    devid = ROOT_DEV;

    root_fs.device = dev;
    root_fs.sb = &root_sb;
    root_fs.type = FS_TYPE;

    dops.dev_init = blk_dev_init;
    dops.dev_release = blk_dev_release;
    dops.dev_read = blk_dev_io_read;
    dops.dev_write = blk_dev_io_write;
    ops.open = root_fs_open;
    ops.write = root_fs_write;
    ops.read = root_fs_read;
    ops.close = root_fs_close;

    dev->dops = &dops;
    dev->fops = &ops;
    register_device(dev, DEVICE_NAME, devid, S_IFREG);

    dev->dops->dev_init();

    ino = get_inode(ROOT_INODE_NUM, dev);
    ino->i_count += 999;
    root_fs.sb->s_iroot = ino;
    current_proc->fp_workdir = current_proc->fp_rootdir = ino;
}


