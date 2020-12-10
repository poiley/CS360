/*
 * File:  link_unlink.c 
 * By: Benjamin Poile and Lovee Baccus
 * Cpts 360 - Fall 2020
 */
#include "commands.h"

extern int dev, imap, bmap, ninodes, nblocks;

/*
 * Function:  link_file 
 * Author: Lovee Baccus
 * --------------------
 * Description: Creates a hard link of the linkname to the existing miNode
 *              Hard links associate two or more names with the same iNode
 * Params:      char *pathname      The file that we want link to 
 *              char *linkname      the name we are assigning the link, 'nickname' we are assigning file
 * Returns:     flag integer variable indicating sucess
 *
 */
int link_file(char *pathname, char *linkname)
{
    char cpy[40];
    strcpy(cpy, pathname);

    // verifying that the pathname file exists 
    // otherwise we would be trying to give something that doesn't exist a nickname
    int oino = getino(pathname);
    if(oino == 0) {
        printf("Original File does not exist\n");
        return -1;
    }

    // verifying if the pathname is a directory, if it is then we break
    MINODE *omip = iget(dev, oino);
    printf("parent ino = %d \n\n\n", oino);
    if(S_ISDIR(omip->inode.i_mode)) {
        printf("Original Path is Directory: NOT ALLOWED\n");
        return -1;
    }

    // verify if 'nickname' already exists, if it does we can end here
    strcpy(cpy, linkname);
    if(getino(linkname)) {
        printf("File already exists\n");
        return -1;
    }
    char* parent = dirname(linkname);
    char *child = basename(cpy);

    // perform the actual steps to assign the link
    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);

    enter_name(pmip, oino, child);

    omip->inode.i_links_count++;
    omip->dirty = 1;

    iput(pmip);
    iput(omip);

    search(pmip, child);

    return 1;
}

/*
 * Function:    unlink_file 
 * Author:      Lovee Baccus
 * --------------------
 * Description: undoes the hard link that was created by link_file()
 *              is used for hard and soft(sym) links
 * Params:      char *filename      the file that is referred to by the link 
 * Returns:     flag integer variable indicating sucess
 *
 */
int unlink_file(char *filename)
{
    char cpy[128];
    strcpy(cpy, filename);
    int ino = getino(filename);
    
    // validates that the file given exists
    if(ino <= 0){
        printf("[unlink]: File does not exist\n");
        return -1;
    }

    // verifies that the user has the authority to delete a link
    // aka the ownership stands or user is a superuser
    MINODE *mip = iget(dev,ino);
    if(running->uid != mip->inode.i_uid || running->uid != 0) {
        printf("[unlink]: Permission Denied\n");
        return -1;
    }
    
    // the actual steps to seperate the file and the linkname and delete the link name 
    if(S_ISREG(mip->inode.i_mode) || S_ISLNK(mip->inode.i_mode))
    {
        char *parent = dirname(filename);
        char *child = basename(filename);
        int pino = getino(parent);
        MINODE *pmip = iget(dev, pino);
        rm_child(pmip, child);
        pmip->dirty = 1;
        iput(pmip);
        mip->inode.i_links_count--;
        if(mip->inode.i_links_count>0) {
            mip->dirty=1;
        } else {
            for(int i = 0; i < 12 && mip->inode.i_block[i] !=0; i++)
                bdalloc(dev, mip->inode.i_block[i]);
            idalloc(dev, ino);
        }
        iput(mip);
    }
    return 0;
}
