/*
 * File:  open_close_lseek.c 
 * By: Benjamin Poile and Lovee Baccus
 * Cpts 360 - Fall 2020
 */
#include "commands.h"

/*
 * Function: open_file 
 * Author: Ben Poile
 * --------------------
 *  Description: Opens a file in a certain mode
 *  Params:      char* pathname     The pathname of the file being opened
 *               char* mode         The mode the file will be opened in
 *  Return: int i   The inode value of the file
 *             -1   for a failed run
 */
int open_file(char *pathname, char *mode){
    OFT *open;
    MINODE *mip;
    int i, ino, i_mode = atoi(mode);

    printf("[DEBUG] in open_file(): i_mode: %d\n", i_mode);

    if (abs_path(pathname) == 0)
        dev = root->dev;
    else
        dev = running->cwd->dev;

    ino = getino(pathname);
    mip = iget(dev, ino); // get the ino and mip of pathname

    if(i_mode == 0 && !maccess(mip, 'r')) { // char = 'r' for R; 'w' for Write, Read+Write, and Append
        iput(mip); 
        return -1;
    } else {
        if (!maccess(mip, 'w')) { // char = 'r' for R; 'w' for Write, Read+Write, and Append
            iput(mip); 
            return -1;
        }
    }

    if ((mip->inode.i_mode & 0xF000) != 0x8000) // If the file is not a regular file, return failure.
        return -1;

    // check if file is already open with incompatible mode
    if (i_mode > 0){ // if not open for read
        for (i = 0; i < NFD; i++) {
            if (running->fd[i] != NULL && running->fd[i]->minodePtr == mip) {
                if (running->fd[i]->mode > 0) {
                    printf("[DEBUG] in open_file(): %s is already open!\n\n", pathname);
                    return -1;
                }
            } else 
                break; //break to maintain the value of i for returining.
        }
    }
    
    open = (OFT*) malloc(sizeof(OFT)); // build the open fd
    open->mode = i_mode;
    open->refCount = 1;
    open->minodePtr = mip;

    switch (i_mode){ // offset
        case 0: // Read: offset = 0
            open->offset = 0; 
            break;
        case 1: // Write: truncate file to size=0
            truncate_file(mip);
            open->offset = 0;
            break;
        case 2: // Read/Write: Don't truncate, offset = 0
            open->offset = 0;
            break;
        case 3: // Append: offset to size of file
            open->offset = mip->inode.i_size;
            break;
        default:
            printf("[DEBUG] in open_file(): Invalid mode.\n");
            return -1;
    }

    for (i = 0; i < NFD; i++) // find empty fd in running PROC's fd array
        if (running->fd[i] == NULL){
            running->fd[i] = open;
            break;
        }
    
    mip->inode.i_atime = time(0L); // update inode access time by casting the int value 0 to a Long

    if (i_mode > 0) // if not R then update modify time as well
        mip->inode.i_mtime = time(0L);

    mip->dirty = 1;

    return i; // return fd i
}

/*
 * Function: truncate_file 
 * Author: Ben Poile
 * --------------------
 *  Description: Truncates file 
 *  Params:      MINODE *mip     The inode value correlating to the file being truncated
 *  Return: int 
 *               0 for a successful run
 */
int truncate_file(MINODE *mip){
    INODE *ip = &mip->inode;      
    int i;

    for (i=0; i<ip->i_blocks; i++){ // iterate through blocks
        if (ip->i_block[i] != 0)
            bzero(ip->i_block, BLKSIZE); // zero it if it's not empty
        else
            break; // break once we reach empty block (we can assume the following are empty)
    }
    // have to do indirect and double-indirect blocks as well
    
    ip->i_mtime = time(0L); // update modified time
    ip->i_size = 0;
    mip->dirty = 1;

    return 0;
}

/*
 * Function: close_file 
 * Author: Ben Poile
 * --------------------
 *  Description: Closes a file after being written to.
 *  Params:      int fd     The file descriptor of the file being closed
 *  Return: int  0 for a successful run
 *              -1 for a failed run
 */
int close_file(int fd) {
    if(fd < 0 || fd >=NFD) { // See if the file descriptor exists
        printf("[DEBUG] in close_file(): File out of range.\n");
        return -1;
    }

    if(running->fd[fd] == NULL) {
        printf("[DEBUG] in close_file(): File does not exist.\n");
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

/*
 * Function: lseek_file 
 * Author: Ben Poile
 * --------------------
 *  Description: Determines the offset required for correctly reading in the next block, and updates the current position.
 *  Params:      int fd         The file descriptor of the file we are starting to read from
 *               int position   The position of where we're currently reading.
 *  Return: int  off for a successful run. It is equal to the offset required considering the current position.
 *                -1 for a failed run
 */
int lseek_file(int fd, int position) {
    if(fd < 0 || running->fd[fd] == NULL) {
        printf("[ERROR] in lseek_file(): Cannot locate file.\n");
        return -1;
    }

    if(position > 0) {
        if(position >running->fd[fd]->minodePtr->inode.i_size) {
            printf("[ERROR] in lseek_file: Position is larger than the size of fd.\n");
            return -1;
        }
        int off = running->fd[fd]->offset;
        running->fd[fd]->offset = position;
        return off;
    }

    printf("[ERROR] in lseek_file(): Postion less than 0.\n");
    
    return 0;
}
