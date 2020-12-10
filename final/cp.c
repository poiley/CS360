/*
 * File:  cp.c 
 * By: Benjamin Poile and Lovee Baccus
 * Cpts 360 - Fall 2020
 */

#include "commands.h"

/*
 * Function: cp_file
 * Author: Ben Poile
 * --------------------
 *  Description: Copies file from source directory to destination directory
 *  Params:      char   *src    File to copy
 *               char  *dest    Destination to copy to
 *  Returns: int    
 *               0 if successful
 */
int cp_file(char *src, char *dest) {
    int n = 0, fd = open_file(src, "0"), gd = open_file(dest, "1");
    char buf[BLKSIZE];

    buf[BLKSIZE] = 0; // terminate it

    if(gd < 0) {
        printf("[DEBUG] in cp_file(): Creating file %s\n", dest);
        creat_file(dest);
        gd = open_file(dest, "1");;
    }
    
    while((n = myread(fd, buf, BLKSIZE))) //Need read to complete
        mywrite(gd, buf, n);
    
    close_file(fd);
    close_file(gd);
    
    return 0;
}
