#include "../level1/commands.h"

int write_file() {
    char string[BLKSIZE];
    int fd;
    pfd();
    printf("[write_file]: enter fd = ");
    scanf("%d", &fd);
    printf("[write_file]: enter text = ");
    scanf("%s", string);
    if(fd < 0 || fd >=NFD)
        return -1;
    if(running->fd[fd] == NULL)
        return -1;
    if(running->fd[fd]->mode == 1 || running->fd[fd]->mode == 2)
        return(mywrite(fd, string, strlen(string)));
    printf("[write_file]: Can't write to a file designated for read\n");
    return -1;
}

int mywrite(int fd, char buf[], int nbytes) {
    printf("[mywrite]: %d %d\n", (int)strlen(buf), nbytes);
    int count = nbytes, blk, dblk;
    int ibuf[256], dbuf[256], buf13[256];
    char wbuf[BLKSIZE];
    OFT *oftp;
    oftp = running->fd[fd];
    MINODE *mip;
    mip = oftp->minodePtr;
    while(nbytes > 0) {
        int lbk = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;
        if(lbk <12) {
            printf("[mywrite]: Direct Block\n");
            if(mip->inode.i_block[lbk] == 0)
                mip->inode.i_block[lbk] = balloc(mip->dev);
\            blk  = mip->inode.i_block[lbk];
        } else if(lbk >= 12 && lbk < 256 + 12) {
            printf("[mywrite]: Indirect Block\n");
            if(mip->inode.i_block[12] == 0) {
                mip->inode.i_block[12] = balloc(mip->dev);
                get_block(mip->dev, mip->inode.i_block[12], (char *)ibuf);
                bzero(ibuf, 256);
                put_block(mip->dev,mip->inode.i_block[12], (char *)ibuf);
            }
            get_block(mip->dev, mip->inode.i_block[12], (char *)ibuf);
            blk = ibuf[lbk -12];
            if(blk ==0) {
                mip->inode.i_block[12] = balloc(mip->dev);
                ibuf[lbk -12] = mip->inode.i_block[lbk];
            }
        } else {
            printf("[mywrite]: Double Indirect Block\n");
            lbk -= (256+12);
            get_block(mip->dev,mip->inode.i_block[13], (char *)buf13);
            dblk = buf13[lbk/256];
            get_block(mip->dev, dblk, (char *)dbuf);
            blk = dbuf[lbk %256];
        }
        memset(wbuf, 0, BLKSIZE);
        get_block(mip->dev, blk, wbuf);
        char *cp = wbuf + startByte;
        int remain = BLKSIZE -startByte;
        char *cq = buf; 
        while(remain > 0) {
            *cp++ = *cq++;
            nbytes--; remain--;
            oftp->offset++;
            if(oftp->offset > mip->inode.i_size)
                mip->inode.i_size++;
            if(nbytes <= 0)
                break;
        }
        put_block(mip->dev,blk,wbuf);
    }
    mip->dirty =1;
    printf("[mywrite]: Wrote %d char into file descriptor fd=%d\n", count,fd);
    return nbytes;
}