/*
 * File:  mv.c 
 * By: Benjamin Poile and Lovee Baccus
 * Cpts 360 - Fall 2020
 */
#include "commands.h"

/*
 * Function: mv_file
 * Author: Ben Poile
 * --------------------
 *  Description: Moves file from source directory to destination directory
 *  Params:      char   *src    File to copy
 *               char  *dest    Destination to copy to
 *  Returns: int    
 *               0 if successful
 */
int mv_file(char *src, char *dest) {
    printf("[DEBUG] in mv_file(): Starting mv\n");

    int sfd = open_file(src, "0");
    MINODE *mip = running->fd[sfd]->minodePtr;
    if(mip->dev == sfd) { // Same Dev link then remove src
        printf("[DEBUG] in mv_file(): Same dev, linking %s to %s\n", src, dest);
        link_file(src, dest);
        unlink_file(src);
    } else { // Different dev use cp
        printf("[DEBUG] in mv_file(): Different dev, copying %s to %s\n", src, dest);
        cp_file(src, dest);
        unlink_file(src);
    }
    
    return 0;
}
