/*
 * File:  write.c 
 * By: Benjamin Poile and Lovee Baccus
 * Cpts 360 - Fall 2020
 */
#include "commands.h"

/*
 * Function: write_file
 * Author: Ben Poile
 * --------------------
 *  Description: Allows the user to input data to write to disk
 *  Params: void
 *  Returns: int    
 *               nbytes written to the disk if successful
 *                   -1 if failed run
 */
int write_file() {
    char string[BLKSIZE];
    int fd;
    pfd();
    printf("[DEBUG] in write_file(): enter fd = ");
    scanf("%d", &fd);
    printf("[DEBUG] in write_file(): enter text = ");
    scanf("%s", string);
    if(fd < 0 || fd >=NFD)
        return -1;
    if(running->fd[fd] == NULL)
        return -1;
    if(running->fd[fd]->mode == 1 || running->fd[fd]->mode == 2)
        return(mywrite(fd, string, strlen(string)));
    printf("[DEBUG] in write_file(): Can't write to a file designated for read\n");
    return -1;
}

/*
 * Function: mywrite
 * Author: Ben Poile
 * --------------------
 *  Description: Writes an nbytes amount of bytes from the memory pointed to by buf, into the file correlating to the file descriptor value.
 *  Params:     int    fd   File descriptor of the file to be written to
 *              char buf[]  Point in memory
 *              int nbytes  Number of bytes to read in from the buffer
 *  Returns: int    
 *               nbytes written to the disk if successful
 *                   -1 if failed to run
 */
int mywrite(int fd, char buf[], int nbytes){
    printf("[DEBUG] in mywrite(): %d %d\n", (int)strlen(buf), nbytes);
    int count = nbytes, blk, dblk;
    int ibuf[256], dbuf[256], buf13[256];
    char wbuf[BLKSIZE];
    OFT *oftp;
    oftp = running->fd[fd];
    MINODE *mip;
    mip = oftp->minodePtr;

    while(nbytes > 0) {
        int lbk = oftp->offset / BLKSIZE; // Logical block
        int startByte = oftp->offset % BLKSIZE; // Start Byte

        if(lbk <12) { // Direct Block
            printf("[DEBUG] in mywrite(): Direct Block\n");
            if(mip->inode.i_block[lbk] == 0) // If no data block is present, allocate one
                mip->inode.i_block[lbk] = balloc(mip->dev);
            blk = mip->inode.i_block[lbk]; // Saves Block into blk
        } else if(lbk >= 12 && lbk < 256 + 12) { // Indirect Block
            printf("[DEBUG] in mywrite(): Indirect Block\n");
            if(mip->inode.i_block[12] == 0) { // Like above, if no data block allocate one
                mip->inode.i_block[12] = balloc(mip->dev); // Allocate block
                get_block(mip->dev, mip->inode.i_block[12], (char *)ibuf); // Get block into memmory
                bzero(ibuf, 256); // Zero it out
                put_block(mip->dev, mip->inode.i_block[12], (char *)ibuf); // Put block into memory
            }
            get_block(mip->dev, mip->inode.i_block[12], (char *)ibuf); // Get block into ibuf
            blk = ibuf[lbk - 12]; // Set blk to disk block
            if(blk == 0) { // If 0 allocate it
                mip->inode.i_block[12] = balloc(mip->dev);
                ibuf[lbk -12] = mip->inode.i_block[lbk];
            }
        } else { // Double indirect blocks
            printf("[DEBUG] in mywrite(): Double Indirect Block\n"); // From the help posted by KC
            lbk -= (256+12);
            get_block(mip->dev,mip->inode.i_block[13], (char *)buf13);
            dblk = buf13[lbk/256];
            get_block(mip->dev, dblk, (char *)dbuf);
            blk = dbuf[lbk %256];
        }

        memset(wbuf, 0, BLKSIZE); // Reset wbuf as to not add too many characters in the last buffer
        get_block(mip->dev, blk, wbuf); // Get data block blk into wbuf
        char *cp = wbuf + startByte; // Set the start point of wbuf
        int remain = BLKSIZE -startByte;
        char *cq = buf; 
        while(remain > 0) { // Write until remain == 0
            *cp++ = *cq++;
            nbytes--; remain--;
            oftp->offset++;
            // If offset is greater than size increase file size
            if(oftp->offset > mip->inode.i_size)
                mip->inode.i_size++;
            // Break when you have read all the bytes
            if(nbytes <= 0)
                break;
        }
        // Put wbuf into data block blk
        put_block(mip->dev,blk,wbuf);
    }
    // Mark as dirty to save when done
    mip->dirty =1;
    printf("[mywrite]: Wrote %d char into file descriptor fd=%d\n", count,fd);
    return nbytes;
}
