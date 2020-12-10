/*
 * File:  read_cat.c 
 * By: Benjamin Poile and Lovee Baccus
 * Cpts 360 - Fall 2020
 */

#include "commands.h"

/*
 * Function: read_file 
 * Author: Ben Poile
 * --------------------
 *  Description: Reads a file descriptor into the buffer.
 *  Params:      char *fd       The path of the file we're reading from
 *               char *bytes    The size of the file we're reading
 *  Return: int  off for a successful run. It is equal to the offset required considering the current position.
 *                -1 for a failed run
 */
int read_file(char *fd, char *bytes){
    int i_fd = running->fd[getino(fd)], i_bytes = atoi(bytes); // Converts the file descriptor string to the ID of the fd 
    char buf[BLKSIZE];
    printf("[DEBUG] in read_file(): Reading fd: %d\n", i_fd);

    return myread(i_fd, buf, i_bytes);
}

/*
 * Function: myread 
 * Author: Ben Poile
 * --------------------
 *  Description: Reads a file into the buffer.
 *  Params:     int     fd      The file descriptor id of the file we're reading from
 *              char  *buf      The size of the file we're reading
 *              int nbytes      The number of bytes to be read.
 *  Return:     int  count on a successful run. Count is equal to the nbytes that were available to read.  
 *                      -1 for a failed run
 */
int myread(int fd, char *buf, int nbytes){
    char readbuf[BLKSIZE];
    int min, count=0, blk, lblk, dblk, start, remain, avail, ibuf[256], dbuf[256];

    if (running->fd[fd] == NULL){ // make sure fd exists
        //printf("[ERROR] in myread(): fd is NULL");
        return -1;
    }

    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;

    if (!mip || !oftp) 
        return -1;

    avail = mip->inode.i_size - oftp->offset;

    //printf("[DEBUG] in myread(): fd=%d offset=%d bytes=%d\n", fd, oftp->offset, nbytes);

    while (nbytes && avail){ // read loop
        lblk = oftp->offset / BLKSIZE;
        start = oftp->offset % BLKSIZE;

        if (lblk < 12){ // direct blocks
            //printf("[DEBUG] in myread(): direct block\n");
            blk = mip->inode.i_block[lblk];
        } else if (lblk >= 12 && lblk < 256 + 12){ // indirect blocks
            //printf("[DEBUG] in myread(): indirect block\n");

            get_block(mip->dev, mip->inode.i_block[12], (char*)ibuf); // from book
            blk = ibuf[lblk-12];
        } else {
            //printf("[DEBUG] in myread(): double indirect block\n");
            lblk -= 268;
            int buf13[256];
            get_block(mip->dev, mip->inode.i_block[13], (char*)buf13);
            dblk = buf13[lblk/256];
            get_block(mip->dev, dblk, (char*)dbuf);
            blk = dbuf[lblk % 256];
        }

        get_block(mip->dev, blk, readbuf);
        char *cp = readbuf + start; // takes care of offset
        remain = BLKSIZE - start;

        // read optimization
        min = smallest(nbytes, avail, remain);
        //printf("[DEBUG] in myread(): Offset: %d\tMin: %d\tBlock: %d\n", oftp->offset, min, blk);
        strncpy(buf, cp, min);

        oftp->offset += min;
        count += min;
        avail -= min;
        nbytes -= min;
        remain -= min;

        //printf("[DEBUG] in myread(): nbytes: %d\tlen(buf): %d", count, (int)strlen(buf));
        //printf("File Contents:\n%s\n", buf);
    }

    return count;
}

/*
 * Function: cat_file 
 * Author: Ben Poile
 * --------------------
 *  Description: Print the contents of a file.
 *  Params: char *filename      The directory of the file to be printed
 *  Return: int
 *                  0 on succesful run
 *                 -1 on a failed run
 */
int cat_file(char *filename){
    char mybuf[BLKSIZE];
    int len = 0, n, i, fd = open_file(filename, "0");

    if (fd < 0) 
        return -1;

    mybuf[BLKSIZE]=0; // terminate mybuf

    printf("File Output:\n\n");

    while ((n = myread(fd, mybuf, BLKSIZE))){
        mybuf[n]=0;

        for (i = 0; i < n; i++){
            if (mybuf[i] == '\\' && mybuf[i++] == 'n') {
                putchar('\n');
                continue;
            }

            putchar(mybuf[i]);
        }

        len += n;
    }
    printf("[DEBUG] in cat_file(): Read %d bytes.\n\n", len);

    close_file(fd);

    return 0;
}
