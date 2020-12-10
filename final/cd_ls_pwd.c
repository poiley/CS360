/*
 * File:  cd_ls_pwd.c 
 * By: Benjamin Poile and Lovee Baccus
 * Cpts 360 - Fall 2020
 */

#include "commands.h"

char t1[9] = "xwrxwrxwr", t2[9] = "---------";

/*
 * Function: cd
 * Author: Ben Poile
 * --------------------
 *  Description: Change the current working directory
 *  Params:      char *pathname     The path we want to change our current working directory to
 *  Returns: int    
 *               0 if successful
 */
int cd(char *pathname) {
    MINODE *mip;
    int ino;

    if (strlen(pathname) > 0)
        ino = getino(pathname);
    else
        ino = root->ino;

    mip = iget(dev, ino);
    
    if ((mip->inode.i_mode & 0xF000) == 0x4000){ // Ensure the path given is a valid directory
        iput(running->cwd);
        running->cwd = mip;
    }

    printf("\n[DEBUG] CWD: "); // Output debugging information on the new current working directory
    int cur_dev = dev;
    rpwd(running->cwd);
    dev = cur_dev;
    putchar('\n');

    return 0;
}

/*
 * Function: ls_file
 * Author: Ben Poile
 * --------------------
 *  Description: Print metadata about the current file being Listed by ls_dir
 *  Params:      MINODE *mip    The inode object of the file we want metadata about
 *               char *name     The path of the file we want metadata about
 *  Returns: int    
 *               0 if successful
 */
int ls_file(MINODE *mip, char *name) {
    int i;
    time_t time;
    char temp[64], l_name[128], buf[BLKSIZE];

    INODE *ip = &mip->inode;
    putchar('\n'); // Start on new line
    
    if ((ip->i_mode & 0xF000) == 0x8000)       // File is of standard type
        printf(" -");
    else if ((ip->i_mode & 0xF000) == 0x4000)  // File is of type directory
        printf(" d");
    else if ((ip->i_mode & 0xF000) == 0xA000){ // File is of type link
        printf(" l");
        get_block(mip->dev, mip->inode.i_block[0], buf);
        strcpy(l_name, buf);
        put_block(mip->dev, mip->inode.i_block[0], buf);
        l_name[strlen(l_name)] = 0;
    }

    for (i = 8; i >= 0; i--) // permissions
        if (ip->i_mode & (1 << i))
            putchar(t1[i]);
        else
            putchar(t2[i]);

    putchar(' ');

    printf("links: %d ", ip->i_links_count); // link count
    printf("uid: %d ", ip->i_uid);           // Owner ID
    printf("gid: %d ", ip->i_gid);           // Group ID
    printf("%6dB ", ip->i_size);             // Size in bytes
    time = (time_t)ip->i_ctime;
    strcpy(temp, ctime(&time));
    temp[strlen(temp) - 1] = 0;
    printf("%s ", temp);                     // The current time, calculated by finding the inode change time.
    printf("Name: %s", name);

    if (S_ISLNK(ip->i_mode) != 0)     // Symbolic link handling
        printf(" -> %s (symlink)", l_name);

    iput(mip);
    
    return 0;
}

/*
 * Function: ls_dir
 * Author: Ben Poile
 * --------------------
 *  Description: Recurse through a directory, printing information about each file.
 *  Params:      MINODE *mip    The inode object of the folder we want to recurse through
 *  Returns: int    
 *               0 if successful
 */
int ls_dir(MINODE *mip) {
    char buf[BLKSIZE], temp[256], *cp;
    DIR *dp;
  
    // Assume DIR has only one data block i_block[0]
    get_block(dev, mip->inode.i_block[0], buf); 
    dp = (DIR *)buf;
    cp = buf;

    printf("[DEBUG] in ls_dir(): mip->ino: %d\n", mip->ino);
    if (mip->ino == 0)
        return 0;
  
    while (cp < buf + BLKSIZE){
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
	
        MINODE *fmip = iget(dev, dp->inode);
        fmip->dirty = 0;
        ls_file(fmip, temp);

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
    putchar('\n');
    putchar('\n');

    iput(mip);

    return 0;
}

/*
 * Function: ls
 * Author: Ben Poile
 * --------------------
 *  Description: Lists the path given
 *  Params:      char *pathname     The directory to list, as a string
 *  Returns: int    
 *               0 if successful
 */
int ls(char *pathname) {
    u32 *ino = malloc(32);      // get ino value
    findino(running->cwd, ino); // assign the ino object just created to the cwd

    if (strlen(pathname) > 0){
        printf("[DEBUG] Running ls on path: %s\n", pathname);
        *ino = getino(pathname);
    }
    
    if (ino != 0) // if the ino correlates to a valid file, ls it.
        ls_dir(iget(dev, *ino));

    return 0;
}

/*
 * Function: rpwd
 * Author: Ben Poile
 * --------------------
 *  Description: Recursively print the working directory
 *  Params:      MINODE *mip    The directory recurse through
 *  Returns: int    
 *               0 if successful
 */
char *rpwd(MINODE *wd){
    MINODE *pip, *newmip;
    int p_ino=0, i;
    u32 ino;
    char my_name[256];
    MTABLE *mntptr;
    if (wd == root) 
        return 0;

    // if at root of other dev, must hop to root dev mount point
    if (wd->dev != root->dev && wd->ino == 2){
        for (i=0; i<NMTABLE; i++)
            if ((mntptr=&mtable[i])->dev == wd->dev)
                break;
        
        newmip = mntptr->mntDirPtr;
        iput(wd);
        p_ino = findino(newmip, &ino);
        wd = iget(newmip->dev, ino);
        pip = iget(newmip->dev, p_ino);
        dev = newmip->dev;
    }

    p_ino = findino(wd, &ino);
    pip = iget(dev, p_ino);

    findmyname(pip, ino, my_name);

    rpwd(pip);
    printf("/%s", my_name);

    iput(pip);

    return 0;
}

/*
 * Function: pwd
 * Author: Ben Poile
 * --------------------
 *  Description: Print the working directory
 *  Params:      MINODE *mip    The directory to print
 *  Returns: int    
 *               0 if successful
 */
char *pwd(MINODE *wd){
    int cur_dev = dev;

    if (wd == root){
        printf("PWD is /\n");
        return 0;
    }

    printf("PWD is ");
    rpwd(wd);
    putchar('\n');

    dev = cur_dev; // Ensure dev is set back to the current.

    return 0;
}
