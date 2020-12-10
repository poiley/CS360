/*
 * File:  mkdir_creat.c 
 * By: Benjamin Poile and Lovee Baccus
 * Cpts 360 - Fall 2020
 */

#include "commands.h"

/* Function:    make_dir 
 * Author:      Lovee Baccus
 * --------------------
 * Description: creates a directory -- adds a directory to the file tree              
 * Params:      char *pathname      pathname as string 
 * Returns:     the iNode value of the file or 0 if failed
 */
int make_dir(char *pathname) {
    char *path, *name, cpy[128];
    int pino, r, dev;
    MINODE *pmip;

    // need a copy for modification purposes
    strcpy(cpy, pathname); 

    // verifies if we are at the root or in a current working directory
    if (abs_path(pathname)==0) {
        pmip = root;
        dev = root->dev;
    } else {
        pmip = running->cwd;
        dev = running->cwd->dev;
    }

    // parsing the pathname
    path = dirname(cpy);
    name = basename(pathname);

    // initializing the miNode object
    pino = getino(path);
    pmip = iget(dev, pino);

    // verifies permissions
    if(!maccess(pmip, 'w'))
    {
         printf("FAILED\n");
        iput(pmip);
        return 0;
    }
    
    // using mymkdir to create the directorry and store the miNode in it
    // and adding that directory to the file tree in the appropriate place
    if ((pmip->inode.i_mode & 0xF000) == 0x4000){ // is_dir
        printf("pmip must be dir\n");
        if (search(pmip, name) == 0){ // if can't find child name in start MINODE
            r = mymkdir(pmip, name);
            pmip->inode.i_links_count++; // increment link count
            pmip->inode.i_atime = time(0L); // touch atime
            pmip->dirty = 1; // make dirty
            iput(pmip); // write to disk

            printf("\nDirectory %s Added!\n", pathname);
            return r;
        } else {
            printf("\nDirectory %s already exists!\n", name);
            iput(pmip);
        }
    }

    iput(pmip);

    return 0;
}

/* Function:    mymkdir 
 * Author:      Lovee Baccus
 * --------------------
 * Description: creates a directory memory block
 *              helper function for make_dir
 * Params:      char *pip       the miNode that we are putting into the directory that we are creating
 *              char *name      name of directory to be created
 * Returns:     the iNode value of the file or 0 if failed
 */
int mymkdir(MINODE *pip, char *name){
    char buf[BLKSIZE], *cp;
    DIR *dp;
    MINODE *mip;
    INODE *ip;
    int ino = ialloc(dev), bno = balloc(dev), i;
    printf("ino=%d bno=%d\n", ino, bno);

    mip = iget(dev, ino);
    ip = &mip->inode;

    char temp[256];
    findmyname(pip, pip->ino, temp);
    printf("ino=%d name=%s\n", pip->ino, temp);

    ip->i_mode = 0x41ED; // set to dir type and set perms
    ip->i_uid = running->uid; // set owner uid
    ip->i_gid = running->gid; // set group id
    ip->i_size = BLKSIZE; // set byte size
    ip->i_links_count = 2; // . and ..
    ip->i_blocks = 2; // each block = 512 bytes
    ip->i_block[0] = bno; // new dir has only one data block
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time

    for (i=1; i <= 14; i++)
        ip->i_block[i] = 0;
    // write mip to disk
    // make dirty
    mip->dirty = 1; 
    
    iput(mip); // write to disk
    bzero(buf, BLKSIZE);
    cp = buf;

    dp = (DIR*)cp; 
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';

    cp += dp->rec_len;
    dp = (DIR *)cp; 

    dp->inode = pip->ino;
    dp->rec_len = BLKSIZE-12;
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';

    put_block(dev, bno, buf); 
    printf("Written to disk %d\n", bno);
    enter_name(pip, ino, name);

    return 0;
}

/* Function:    enter_name 
 * Author:      Lovee Baccus
 * --------------------
 * Description: Adds the newly created folder to the list of the parent directory's Children, 
 *              thus becoming a part of the file system tree.
 *              its not enought for the new node to be aware of itself, the surrounding generations 
 *              need to know it exists, so we go up a level 
 * Params:      miNode *pip     parent directory
 *              int myino       ino of the child directory
 *              char *myname    name of child directory
 * Returns:     int 0 if successful
 */
int enter_name(MINODE *pip, int myino, char *myname) {
    char buf[BLKSIZE], *cp, temp[256];
    DIR *dp;
    int block_i, i, ideal_len, need_len, remain, blk;

    need_len = 4 * ((8 + (strlen(myname)) + 3) / 4);
    printf("Need len %d for %s\n", need_len, myname);

    for (i=0; i<12; i++){ // find empty block
        if (pip->inode.i_block[i]==0) break;
        get_block(pip->dev, pip->inode.i_block[i], buf); // get that empty block
        printf("get_block(i) where i=%d\n", i);
        block_i = i;
        dp = (DIR *)buf;
        cp = buf;

        blk = pip->inode.i_block[i];

        while (cp + dp->rec_len < buf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;

            printf("[%d %s] ", dp->rec_len, temp);
            putchar(', ');         
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        printf("[%d %s]\n", dp->rec_len, dp->name);

        ideal_len = 4 * ((8 + dp->name_len + 3) / 4);

        printf("ideal_len=%d\n", ideal_len);
        remain = dp->rec_len - ideal_len;

        if (remain >= need_len) {
            dp->rec_len = ideal_len; // trim last rec_len to ideal_len

            cp += dp->rec_len;
            dp = (DIR*)cp;
            dp->inode = myino;
            dp->rec_len = remain;
            dp->name_len = strlen(myname);
            strcpy(dp->name, myname);
        }
    }

    printf("put_block : i=%d\n", block_i);
    put_block(pip->dev, pip->inode.i_block[block_i], buf);
    printf("put parent block to dist=%d to disk\n", blk);

    return 0;
}

/* 
 * Function:    creat_file 
 * Author:      Lovee Baccus
 * --------------------
 * Description: Creates a new file and adds it to the filesystem, assuming it doesn't already exist 
 * Params:      char *pathname      The pathname of the file to be created, as a string
 * Returns:     int
 *                  result for a successful run, where result represents the inode value of the created file.
 *                      -1 for a failed run
 */
int creat_file(char *pathname) {
    printf("[DEBUG] in creat_file(): pathname is %s\n", pathname);
    if (!pathname[0]) { //if path is null then display error and return fail
        printf("[ERROR] in creat_file(): path name not specified\n");
        return -1;
    }

    if (pathname[0] == '/') //initialize device depending on absolute or relative path
        dev = root->dev;
    else
        dev = running->cwd->dev;

    char *directory, *base;

    directory = dirname(pathname);
    base = basename(pathname);

    int pino = getino(directory); //detmine parent inode number
    if (pino < 0)                       //if parent directory not found return fail
        return -1;

    MINODE *pmip = iget(dev, pino);   // get parent minode
    if (!S_ISDIR(pmip->inode.i_mode)) { //if parent minode is not a directory display error and return fail
        printf("[ERROR] in creat_file(): %s is not a directory\n", directory);
        iput(pmip);
        return -1;
    }

    printf("[DEBUG] in creat_file(): Searching for '%s'.\n", base);
    
    if (search(pmip, base) != -1) { //if name already exists display error and return fail
        printf("[ERROR] in creat_file(): %s already exists\n", base);
        quit();
        iput(pmip);
        return -1;
    }

    pmip->inode.i_atime = time(0L); //update parent minode
    pmip->dirty = 1;

    int result = mycreat(pmip, base); //return the success or fail of creating the file

    iput(pmip); //put parent minode back

    return result;
}

/* 
 * Function:    mycreat 
 * Author:      Lovee Baccus
 * --------------------
 * Description: creates a file in memory
 *              helper function for creat_file()
 * Params:      miNode *pip     the miNode that we are putting into the file that we are creating
 *              char *name      name of the file to create
 * Returns:     flag int indicating success
 */
int mycreat(MINODE *pip, char *name) {
    MINODE *mip;
    INODE *ip;
    int ino = ialloc(dev);

    mip = iget(dev, ino);
    ip = &mip->inode;
    ip->i_mode = 0x81A4; // set to file type and set perms
    ip->i_uid = running->uid; // set owner uid
    ip->i_gid = running->gid; // set group id
    ip->i_size = 0; // no data blocks
    ip->i_links_count = 1; // . and ..
    //ip->i_blocks = 2; // each block = 512 bytes
    //ip->i_block[0] = bno; // new dir has only one data block
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time

    mip->dirty = 1;
    printf("[DEBUG] in mycreat(): Successfully wrote inode to the disk!\n");

    enter_name(pip, ino, name);

    iput(mip);

    return 0;
}
