/*
 * File:  symlink.c 
 * By: Benjamin Poile and Lovee Baccus
 * Cpts 360 - Fall 2020
 */
#include "commands.h"

/* Function:  sym_link 
 * Author: Lovee Baccus
 * --------------------
 * Description: creates a soft link of the linkname to the existing miNode
 *              creates an indirect pointer to a file or directory, is a 'shortcut'
 * Params:      char *src       The file that we want link to, the source file
 *              char *dest      the name we are assigning the link, 'nickname' we are assigning file
 * Returns:     int 0 for successful run
 */
int sym_link(char *src, char *dest){
    int sino, dino;
    char *name, temp[128], buf[BLKSIZE];
    MINODE *mip;

    strcpy(temp, src);
    name = basename(src);
    name[strlen(name)] = 0;

    sino = getino(src);   // source ino
    dino = getino(dest);  // destation ino

    // verifies the source file exists and the dest file (nickname) is ready to be used
    if (sino > 0 && dino == 0) {
        creat_file(dest);
        dino = getino(dest);
        printf("ino we want=%d\n", dino);

        mip = iget(dev, dino);  // creates a new miNode object
        mip->inode.i_mode = 0127777;    // sets file type to link
        get_block(mip->dev, mip->inode.i_block[0], buf);    // copies over the source file attributes to the newly 
                                                            //created file of tyle link
        strcpy(buf, name);
        put_block(mip->dev, mip->inode.i_block[0], buf);
        mip->inode.i_size = strlen(name);   // modify size to = old file name
        mip->dirty = 1;
        iput(mip);
    } else if (sino <= 0)
        printf("The source already exists!");
    else if (dino != 0)
        printf("The destination already exists!");

    return 0;
}
