#include <stdio.h>
#include <string.h>
#include "makefs.h"

char disk[totalsize+1];

int makefs()
{
    char *super_block = "00000077696E697857494E49585F524F4F544653000000000000000000000000000000000000010000000000000000F0000000000000010000000000000001EF0000000000000FEF00000000000001000000000000000080000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    char *super_block2 = "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    char *block_bitmap = "FFFFFFFFFFFFFFFF0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    char *block_bitmap2 = "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    char *inode_bitmap = "C0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    char *inode_bitmap2 = "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    int block_size = 1024;
    
    char *pdisk = disk;
    int sb = 1024;
    int blockmap = 1024;
    int inodemap = 1024;
    int inode_tablesize = 496*128; //at 64th block
    int first_free_block = (sb + blockmap + inodemap + inode_tablesize) / 1024 +1;
    long remaining = totalsize - sb - blockmap - inodemap - inode_tablesize;
    int i=0;

    strcat(pdisk,super_block);
    strcat(pdisk,super_block2);
    strcat(pdisk,block_bitmap);
    strcat(pdisk,block_bitmap2);
    strcat(pdisk,inode_bitmap);
    strcat(pdisk,inode_bitmap2);
    while(*pdisk++);
    pdisk--;

    // printf("%d %d %d %d\n",strlen(super_block),strlen(block_bitmap), strlen(block_bitmap2),strlen(inode_bitmap) );
    // printf("%s%s%s%s",super_block,block_bitmap,block_bitmap2,inode_bitmap);
    for(;i<inode_tablesize;i++){
        if(i==128){ //the first one is left as empty deliberately
            sprintf(pdisk,"%08x",0x41c0); //drwx------
            pdisk += 8;
            for( int j=0; j< 3; j++){ //nlink gid uid set to 0
                sprintf(pdisk,"%08x", 0);
                pdisk += 8;
            }
            sprintf(pdisk, "%08x", 64); //i_size
            pdisk += 8;
            for( int j=0; j< 3; j++){ //atime mtime ctime set to 0
                sprintf(pdisk,"%08x", 0);
                pdisk += 8;
            }
            // printf("first free inode %d\n",first_free_block+1);
            sprintf(pdisk, "%08x",first_free_block);
            pdisk+=8;
            for( int j=0; j< 7; j++){ //remaining zones
                sprintf(pdisk,"%08x", 0);
                pdisk += 8;
            }
            
            i+= 127;
            continue;
        }
        *pdisk++ = 0;
    }
    //block for inode 1, which is the root directory
    sprintf(pdisk,"%08x",1);
    pdisk += 8;
    *pdisk++ = '.';
    *pdisk++ = '\0';
    for( int j=0; j<22; j++){
        *pdisk++ = 0;
    }
    sprintf(pdisk,"%08x",1);
    pdisk += 8;
    *pdisk++ = '.';
    *pdisk++ = '.';
    *pdisk++ = '\0';
    for( int j=0; j<21; j++){
        *pdisk++ = 0;
    }
    i+=64;

    for(;i<inode_tablesize + remaining;i++){
        *pdisk++ = 0;
    }
    *pdisk = '\0';
    // printf("%lu, %d\n",pdisk - disk,totalsize);
    int curr = sb;
    printf("\nsuper block 0 - 0x%08x\n", curr);
    printf("block map 0x%08x - 0x%08x\n",curr, curr+blockmap );
    curr += blockmap;
    printf("inode map 0%08x - 0x%08x\n",curr, curr+inodemap );
    curr += inodemap;
    printf("inode table 0x%08x - 0x%08x\n",curr, curr+inode_tablesize );
    curr += inode_tablesize;
    printf("data block 0x%08x - 0x%lx\n Number of free blocks %ld\n",curr, curr+remaining, remaining/1024 );
    curr += remaining;
    
    return 0;
    // return disk;
}