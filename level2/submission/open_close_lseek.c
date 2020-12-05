#include "globals.h"

#include <stdio.h>

int open_file(char *pathname, char *mode){
    OFT *open;
    MINODE *mip;
    int i, ino, i_mode = atoi(mode);

    printf("[open_file]: i_mode=%d\n", i_mode);

    if (pathname[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;

    ino = getino(pathname);
    mip = iget(dev, ino);

    if(i_mode == 0)
        if (!maccess(mip, 'r')) {
            iput(mip); 
            return -1;
        }
    else
        if (!maccess(mip, 'w')) { 
            iput(mip); 
            return -1;
        }
    
    if ((mip->inode.i_mode & 0xF000) != 0x8000) 
        return -1;

    if (i_mode > 0){ 
        for (i=0; i<NFD; i++){
            if (running->fd[i] != NULL && running->fd[i]->minodePtr == mip){
                if (running->fd[i]->mode > 0){
                    printf("[open_file]: %s already open for write!\n\n", pathname);
                    return -1;
                }
            } else
                break;
        }
    }
    
    open = (OFT*)malloc(sizeof(OFT)); 
    open->mode = i_mode;
    open->refCount = 1;
    open->minodePtr = mip;

    switch (i_mode) { 
        case 0: 
            open->offset = 0; 
            break;
        case 1: 
            truncate_file(mip);
            open->offset = 0;
            break;
        case 2: 
            open->offset = 0;
            break;
        case 3: 
            open->offset = mip->inode.i_size;
            break;
        default:
            printf("[open_file]: Invalid mode!\n");
            return -1;
    }

    for (i = 0; i < NFD; i++) 
        if (running->fd[i] == NULL){
            running->fd[i] = open;
            break;
        }
    
    mip->inode.i_atime = time(0L);

    if (i_mode > 0) 
        mip->inode.i_mtime = time(0L);

    mip->dirty = 1;
    return i;
}

int truncate_file(MINODE *mip){
    INODE *ip = &mip->inode;      
    int i;

    for (i = 0; i < ip->i_blocks; i++) 
        if (ip->i_block[i] != 0)
            bzero(ip->i_block, BLKSIZE); 
        else
            break;
    
    ip->i_mtime = time(0L);
    ip->i_size = 0;
    mip->dirty = 1;

    return 0;
}

int close_file(int fd) {
    if(fd < 0 || fd >= NFD) {
        printf("[close_file]: File out of range\n");
        return -1;
    }

    if(running->fd[fd] == NULL) {
        printf("[close_file]: File does not exist\n");
        return -1;
    }

    OFT *oftp;
    oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->refCount--;
    
    if(oftp->refCount > 0) 
        return 0;
    
    MINODE *mip = oftp->minodePtr;
    iput(mip);
    return 0;
}

int lseek_file(int fd, int position) {
    if(fd < 0 || running->fd[fd] == NULL) {
        printf("[lseek_file]: Cannot Locate file\n");
        return -1;
    }

    if(position > 0) {
        if(position > running->fd[fd]->minodePtr->inode.i_size) {
            printf("[lseek_file]: Position is larger than the size of fd\n");
            return -1;
        }
        
        int off = running->fd[fd]->offset;
        running->fd[fd]->offset = position;
        return off;
    }

    printf("[lseek_file]: Postion less than 0: Not allowed\n");
    return 0;
}
