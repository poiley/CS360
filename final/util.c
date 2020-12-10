/*
 * File:  util.c 
 * By: Benjamin Poile and Lovee Baccus
 * Cpts 360 - Fall 2020
 */

/*********** util.c file ****************/
#include "util.h"

extern PROC *running;
extern MINODE *root, minode[NMINODE];
extern MTABLE mtable[NMTABLE];
extern char gline[128], *name[64];
extern int nname, inode_start, dev, imap, bmap, ninodes, nblocks;
int r;

//Given from book, reads from the lseek on the buf. put_block will write to it. 
int get_block(int dev, int blk, char *buf){
    lseek(dev, (long)blk*BLKSIZE, 0);
    r = read(dev, buf, BLKSIZE);
    return r;
}   

int put_block(int dev, int blk, char *buf){
    lseek(dev, (long)blk*BLKSIZE, 0);
    r = write(dev, buf, BLKSIZE);
    return r;
}   

//FROM THE BOOK
int tokenize(char *pathname){
    int i;
    char *s;

    strcpy(gline, pathname); 
    nname = 0;

    s = strtok(gline, "/");
    while (s) {
        name[nname] = s;
        nname++;
        s = strtok(0, "/");
    }
}

MINODE *iget(int dev, int ino){
    int i;
    MINODE *mip;
    char buf[BLKSIZE];
    int blk, offset;
    INODE *ip;

    for (i=0; i<NMINODE; i++){
        mip = &minode[i];
        if (mip->dev == dev && mip->ino == ino){
            //printf("\nmip=%s refCount++ now=%d\n", mip->ino, mip->refCount);
            mip->refCount++;
            //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
            return mip;
        }
    }
    
    for (i=0; i<NMINODE; i++){
        mip = &minode[i];
        if (mip->refCount == 0){
            //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
            mip->refCount = 1;
            mip->dev = dev;
            mip->ino = ino;

            // get INODE of ino into buf[ ]    
            blk    = (ino-1)/8 + inode_start;
            offset = (ino-1) % 8;

            //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

            get_block(dev, blk, buf);
            ip = (INODE *)buf + offset;
            // copy INODE to mp->INODE
            mip->inode = *ip;
            return mip;
        }
    }   
    printf("PANIC: no more free minodes\n");
    return 0;
}
    
void iput(MINODE *mip){
    int block, offset;
    char buf[BLKSIZE];
    INODE *ip;

    if (mip==0) return;

    //printf("\nmip=%d refCount-- now=%d\n", mip->ino, mip->refCount);
    mip->refCount--;
    if (mip->refCount > 0)  // minode is still in use
        return;
    if (!mip->dirty)        // INODE has not changed; no need to write back
        return;

    /* write INODE back to disk */
    /***** NOTE *******************************************
        For mountroot, we never MODIFY any loaded INODE
        so no need to write it back
        FOR LATER WORK: MUST write INODE back to disk if refCount==0 && DIRTY
        Write YOUR code here to write INODE back to disk
    ********************************************************/

    printf("inode_start is %d\n", inode_start);

    block = (mip->ino - 1) / 8 + inode_start;
    offset = (mip->ino -1) % 8;

    printf("block is %d and offset is %d\n", block, offset);

    get_block(mip->dev, block, buf);
    ip = (INODE *)buf + offset;
    *ip = mip->inode;
    put_block(mip->dev, block, buf);
    mip->refCount = 0;
} 

//FROM BOOK BUT EDITED HOW IT LOOKS 
//SEARCH DIRECTORIES USING MINODE
int search(MINODE *mip, char *name){
    for (int dblk = 0; dblk < 12; dblk++) { //execute across all direct blocks within inode's inode table
        if (!(mip->inode.i_block[dblk])) //if empty block found return fail
            return -1;

        char buf[BLKSIZE];
        get_block(mip->dev, mip->inode.i_block[dblk], buf); //read a directory block from the inode table into buffer
        DIR *dp = (DIR *)buf;                               //cast buffer as directory pointer

        while ((char *)dp < &buf[BLKSIZE]) //execute while there is another directory struct ahead
        {
            if (!strncmp(dp->name, name, dp->name_len) //check if directory name matches name
                && strlen(name) == dp->name_len)       //prevents . == ..
                return dp->inode;                      //return inode number if found

            dp = (char *)dp + dp->rec_len; //point to next directory struct within buffer
        }
    }

    return -1;
}

/*
 * Function: getino 
 * Author: Ben Poile
 * --------------------
 *  Description: Returns the inode value of the given path
 *  Params:      char* pathname     The pathname of the desired file
 *  Return: int i   The inode value of the file
 *             -1   for a failed run
 */
int getino(char *pathname) {
    int i, ino, blk, disp;
    char buf[BLKSIZE];
    INODE *ip;
    MINODE *mip;

    if (strcmp(pathname, "/") == 0)
        return 2;

    // starting mip = root OR CWD
    if (pathname[0] == '/')
        mip = root;
    else
        mip = running->cwd;

    mip->refCount++; // because we iput(mip) later

    tokenize(pathname);

    for (i = 0; i < nname; i++)
    {
        ino = search(mip, name[i]);

        if (ino == 0)
        {
            iput(mip);
            // printf("name %s does not exist\n", name[i]);
            return 0;
        }
        iput(mip);            // release current mip
        mip = iget(dev, ino); // get next mip
    }

    iput(mip); // release mip
    return ino;
}
    
int findmyname(MINODE *parent, u32 myino, char *myname){
    DIR *dp;
    char buf[BLKSIZE], temp[256], *cp;

    get_block(dev, parent->inode.i_block[0], buf);
    dp = (DIR *)buf;
    cp = buf;

    while (cp < buf + BLKSIZE){
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;

        if (dp->inode == myino)
            strcpy(myname, temp);

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }

    return 0;
}

/*
 * Function: findino 
 * Author: Ben Poile
 * --------------------
 *  Description: Returns the inode value of the parent 
 *  Params:      MINODE* mip     A struct of the current working directory
 *               u32   myino     The current working directory's ino
 *  Return: int
 *               dp->inode       The inode value of the parent directory
 */
int findino(MINODE *mip, u32 *myino) {
    char buf[BLKSIZE], *cp;   
    DIR *dp;

    get_block(mip->dev, mip->inode.i_block[0], buf);
    cp = buf; 
    dp = (DIR *)buf;
    *myino = dp->inode;
    cp += dp->rec_len;
    dp = (DIR *)cp;
    return dp->inode;
}

/*
 * Function: abs_path 
 * Author: Ben Poile
 * --------------------
 *  Description: Determines if the path is absolute or relative
 *  Params:      char *path      The path to be read in       
 *  Return: int
 *               0 if the path is absolute
 *              -1 if the path is relative
 */
int abs_path(char *path){
    if (path[0] == '/')
        return 0;
    else
        return -1;
}

/*
 * Function: tst_bit 
 * Author: Lovee Baccus
 * --------------------
 *  Description: Tests the bit value within the byte array
 *  Params:      char  *buf      Byte array
 *               int    bit      Bit location  
 *  Return: int
 *               1 if the bit is in byte array
 *               0 if the bit is not in byte array
 */
int tst_bit(char *buf, int bit){
    int i = bit / 8, j = bit % 8;

    if (buf[i] & (1 << j))
        return 1;

    return 0;
}

/*
 * Function: set_bit 
 * Author: Lovee Baccus
 * --------------------
 *  Description: Sets the bit value within the byte array
 *  Params:      char  *buf      Byte array
 *               int    bit      Bit location  
 *  Return: int
 *               0 if successful
 */
int set_bit(char *buf, int bit){
    int i = bit / 8, j = bit % 8;

    buf[i] |= (1 << j);

    return 0;
}

/*
 * Function: clr_bit 
 * Author: Lovee Baccus
 * --------------------
 *  Description: Clears the bit value within the byte array
 *  Params:      char  *buf      Byte array
 *               int    bit      Bit location  
 *  Return: int
 *               0 if successful
 */
int clr_bit(char *buf, int bit){
    int i = bit / 8, j = bit % 8;

    buf[i] &= ~(1 << j);

    return 0;
}

/*
 * Function: dec_free_inodes 
 * Author: Lovee Baccus
 * --------------------
 *  Description: Decrease the amount of free iNodes on the device
 *  Params:      int    dev      File descriptor of device
 *  Return: int
 *               0 if successful
 */
int dec_free_inodes(int dev) {
    char buf[BLKSIZE];

    get_block(dev, 1, buf); // dec the super table
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf; // dec the GD table
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);

    return 0;
}

/*
 * Function: dec_free_blocks 
 * Author: Lovee Baccus
 * --------------------
 *  Description: Decrease the amount of free blocks on the device
 *  Params:      int    dev      File descriptor of device
 *  Return: int
 *               0 if successful
 */
int dec_free_blocks(int dev){
    char buf[BLKSIZE];

    get_block(dev, 1, buf); // dec the super table
    sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf; // dec the GD table
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);

    return 0;
}

/*
 * Function: inc_free_inodes 
 * Author: Lovee Baccus
 * --------------------
 *  Description: Increase the amount of free inodes on the device
 *  Params:      int    dev      File descriptor of device
 *  Return: int
 *               0 if successful
 */
int inc_free_inodes(int dev){
    char buf[BLKSIZE];

    get_block(dev, 1, buf); // get the super table
    sp = (SUPER*)buf;
    sp->s_free_inodes_count++; // inc free inodes
    put_block(dev, 1, buf); // put it back

    get_block(dev, 2, buf); // get the gd table
    gp = (GD*)buf;
    gp->bg_free_inodes_count++; // inc free inodes
    put_block(dev, 2, buf); // put it back

    return 0;
}

/*
 * Function: inc_free_blocks 
 * Author: Lovee Baccus
 * --------------------
 *  Description: Increase the amount of free blocks on the device
 *  Params:      int    dev      File descriptor of device
 *  Return: int
 *               0 if successful
 */
int inc_free_blocks(int dev){
    char buf[BLKSIZE];

    get_block(dev, 1, buf); 
    sp = (SUPER *)buf;
    sp->s_free_blocks_count++; // dec the super table
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf; // dec the GD table
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);

    return 0;
}

// Allocate inode on device file descriptor
int ialloc(int dev){
    int i;
    char buf[BLKSIZE];

    get_block(dev, imap, buf);

    for (i=0; i < ninodes; i++){
        if (tst_bit(buf, i) == 0){
            set_bit(buf, i);
            put_block(dev, imap, buf);
            dec_free_inodes(dev);
            printf("allocated ino with %d\n", i+1); // bits 0..n, ino 1..n+1
            return i+1;
        }
    }
    return 0;
}

// deallocate inode on device file descriptor
int idalloc(int dev, int ino){
    char buf[BLKSIZE];

    if (ino > ninodes){
        printf("inumber=%d not possible.\n", ino);
        return 0;
    }

    get_block(dev, imap, buf);
    clr_bit(buf, ino-1);
    put_block(dev, imap, buf);

    inc_free_inodes(dev);
    printf("deallocated ino with %d\n", ino);

    return 0;
}

// allocate block on device file descriptor
int balloc(int dev){
    int i;
    char buf[BLKSIZE];

    get_block(dev, bmap, buf);

    for (i=0; i < nblocks; i++){
        if (tst_bit(buf, i) == 0){
            set_bit(buf, i);
            put_block(dev, bmap, buf);
            printf("allocated block in balloc is %d\n", i+1); // bits 0..n, ino 1..n+1
            dec_free_blocks(dev);
            return i+1;
        }
    }
    return 0;
}

// deallocate block on device file descriptor
int bdalloc(int dev, int blk){
    char buf[BLKSIZE];

    if (blk > nblocks){
        printf("bdalloc error\n", blk);
        return 0;
    }

    get_block(dev, bmap, buf);
    clr_bit(buf, blk-1);
    put_block(dev, bmap, buf);

    inc_free_blocks(dev);
    printf("deallocated block is %d\n", blk-1);

    return 0;
}

// Level 2 functions

/*
 * Function: pfd 
 * Author: Ben Poile
 * --------------------
 *  Description: Print the file descriptors of each open file.
 *  Params: void
 *  Return: int
 *                  0 on succesful run
 */
int pfd() {
    int i;
    printf("\n[DEBUG] in pfd():    fd    mode    offset   INODE\n");
    printf(  "[DEBUG] in pfd():   ----  ------  -------- -------\n");
    for(i = 0; i < NFD; i++) {
        if(running->fd[i]){
            OFT *cur = running->fd[i];
            char mode[8];

            switch(cur->mode) {
                case 0:
                    strcpy(mode, "READ"); break;
                case 1:
                    strcpy(mode, "WRITE"); break;
                case 2:
                    strcpy(mode, "R?W"); break;
                case 3:
                    strcpy(mode, "APPEND"); break;
            }
            printf(  "[DEBUG] in pfd():    %d %6s %4d   [%d, %d]\n", i, mode, cur->minodePtr->dev, cur->minodePtr->ino);
        }else
            break; //no more open fds
    }
    putchar('\n');

    return 0;
}

/*
 * Function: faccess 
 * Author: Ben Poile
 * --------------------
 *  Description: Check if a file is accessible in a given mode given the pathname as a string
 *  Params:  char *pathname     The pathname of the file to check
 *           char     *mode     The mode to open the file with
 *  Return: int
 *                  0 on succesful run
 *                 -1 on failed run
 */
int faccess(char *pathname, char mode) {
    char t1[9] = "xwrxwrxwr", t2[9] = "---------";
    char permi[9];
    int offset = 0;
    int ino = getino(pathname);
    MINODE *mip = iget(dev,ino);
    INODE *ip = &mip->inode;

    for (int i = 8; i >= 0; i--) // permissions
        if (ip->i_mode & (1 << i))
            permi[i]=t1[i];
        else
            permi[i]='-';
    
    if(mode == 'w')
        offset = 1;
    if(mode == 'x')
        offset = 2;

    // Sudo
    if(running->uid ==0)
        return 1;

    if(ip->i_uid == running->uid) // Owner
        if(mode == permi[offset])
            return 1;
    else if(ip->i_gid == running->gid) // Same group
        if(mode == permi[offset + 3])
            return 1;
    else // Other
        if(mode == permi[offset + 6])
            return 1;

    return 0;
}

/*
 * Function: faccess 
 * Author: Ben Poile
 * --------------------
 *  Description: Check if a file is accessible in a given mode given the inode struct
 *  Params:  MINODE *pathname     The pathname of the file to check
 *           char        mode     The mode to open the file with
 *  Return: int
 *                  0 on succesful run
 *                 -1 on failed run
 */
int maccess(MINODE *mip, char mode) {
    char t1[9] = "xwrxwrxwr", t2[9] = "---------";
    char permi[9];
    int offset = 0;
    INODE *ip = &mip->inode;

    for (int i=8; i >= 0; i--) // permissions
        if (ip->i_mode & (1 << i))
            permi[i]=t1[i];
        else
            permi[i]='-';
    
    if(mode == 'w')
        offset = 1;
    if(mode == 'x')
        offset = 2;

    // Sudo
    if(running->uid ==0)
        return 1;

    if (ip->i_uid == running->uid) // Owner
        if(mode == permi[offset])
            return 1;
    else if(ip->i_gid == running->gid) // Group
        if(mode == permi[offset + 3])
            return 1;
    else // Other
        if(mode == permi[offset + 6])
            return 1;

    return 0;
}

/*
 * Function: smallest 
 * Author: Ben Poile
 * --------------------
 *  Description: Find the smallest number out of three integers
 *  Params: int a, b, c 
 *  Return: int The smallest of the three
 */
int smallest(int a, int b, int c) {
    if (a <= b && a <= c)
        return a;
    else if (b <= c && b <= a)
        return b;
    else
        return c;
}