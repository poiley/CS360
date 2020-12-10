/*
 * File:  rmdir.c 
 * By: Benjamin Poile and Lovee Baccus
 * Cpts 360 - Fall 2020
 */
#include "commands.h"


/* 
 * Function:    rm_dir 
 * Author:      Lovee Baccus
 * --------------------
 * Description: Removes a directory from the file system          
 * Params:      char *pathname      The pathname of the directory to be deleted
 * Returns:     int 
 *                  0 for a successful run
 */
int rm_dir(char *pathname){
    DIR *dp;
    char buf[BLKSIZE], name[256], temp[256], *cp;
    MINODE *mip, *pmip;
    int i, ino, pino;

    // for string manipulation
    strcpy(temp, pathname);

    ino = getino(pathname);
    if(ino == -1) {
        printf("Error\n");
        return 0;
    }
    mip = iget(dev, ino);

    findmyname(mip, ino, name);
    printf("[DEBUG] in rm_dir(): path: %s\tpino:%d\tpname:%s\n", pathname, mip->ino, name);

    printf("[DEBUG] in rm_dir(): running->uid=%d\tino->i_uid: %d\n", running->uid, mip->inode.i_uid);

    if (running->uid != mip->inode.i_uid || running->uid != 0) // How to check if running PROC is User
        return 0;
        
    printf("[DEBUG] in rm_dir(): running->uid == ino->i_uid\n");

    if ((mip->inode.i_mode & 0xF000) == 0x4000 && mip->refCount <= 1){ // if is a dir and not being used
        printf("[DEBUG] in rm_dir(): inode must be a directory!\n");
        printf("[DEBUG] in rm_dir(): mip link_count should be less than 3. This mip has %d.\n", mip->inode.i_links_count);

        if (mip->inode.i_links_count <= 2){ // could still have reg files
            int actual_links=0;
    
            get_block(dev, mip->inode.i_block[0], buf);
            dp = (DIR*)buf;
            cp = buf;

            while (cp < buf + BLKSIZE){
                actual_links++;

                cp += dp->rec_len;
                dp = (DIR*)cp;
            }

            if (actual_links <= 2){ // good to go
                for (i = 0; i < 12; i++){
                    if (mip->inode.i_block[i]==0)
                        continue;
                    else
                        bdalloc(mip->dev, mip->inode.i_block[i]); // dealloc mip blocks
                }

                idalloc(mip->dev, mip->ino); // dealloc mip inode
                iput(mip); // put it

                u32 *inum = malloc(8);
                pino = findino(mip, inum);
                pmip = iget(mip->dev, pino);
                findmyname(pmip, ino, name); // find the name of the dir to be deleted
                printf("[DEBUG] in rm_dir(): pino: %d\tino: %d\tname: %s\n", pino, ino, name);

                if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0 && strcmp(name, "/") != 0){
                    rm_child(pmip, name); // remove name from parent's dir 
                    pmip->inode.i_links_count--; // dec link count
                    pmip->inode.i_atime = pmip->inode.i_mtime = time(0L); // touch a/mtime
                    pmip->dirty = 1; // mark dirty
                    bdalloc(pmip->dev, mip->inode.i_block[0]);
                    idalloc(pmip->dev, mip->ino);
                    iput(pmip);
                }
            } else
                printf("[DEBUG] in rm_dir(): mip has children with a link_count of %d.\n", actual_links);
        } else
            printf("[DEBUG] in rm_dir(): mip has children with a link_count of %d.n", mip->inode.i_links_count);
    } else if (mip->refCount > 1) {
        printf("[DEBUG] in rm_dir(): mip %d was in use. refCount: %d.\n", mip->ino, mip->refCount);
        iput(mip);
    } else {
        printf("[ERROR] in rm_dir(): mip is not a directory.\n");
        iput(mip);
    }

    return 0;
}

/* 
 * Function:    rm_child 
 * Author:      Lovee Baccus
 * --------------------
 * Description: identifiesthe parents when a child node is deleted -- 
 *              enter_name tells the parent when a child is created, this is the inverse 
 *              controls context when something is deleted
 *              pg 338 
 *              search the parents INODE's data block for the child
 *              delete the chaild from paraent directory
 *                  if its the first data entry, then delete it and shrink size
 *                  if its the last, then just yeet it and add its length to the previous entry
 *                  if its in the middle, delete it and shift all te other entries over to account for that
 * Params:      miNode *pmip   The parent minode struct
 *              char   *name   name as a string 
 * Returns:     int 
 *                  0 for a successful run
 */
int rm_child(MINODE *pmip, char *name) {
    char buf[BLKSIZE], *cp, *rm_cp, temp[256];
    DIR *dp;
    int block_i, i, j, size, last_len, rm_len;

    for (i = 0; i < pmip->inode.i_blocks; i++) {
        if (pmip->inode.i_block[i]==0) 
            break;
        get_block(pmip->dev, pmip->inode.i_block[i], buf);
        printf("[DEBUG] in rm_child(): get_block i: %d\n", i);
        printf("[DEBUG] in rm_child(): name: %s\n", name);
        dp = (DIR*)buf;
        cp = buf;

        block_i = i;
        i = 0;
        j = 0;

        while (cp + dp->rec_len < buf + BLKSIZE) {
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;

            if (!strcmp(name, temp)) {
                i = j;
                rm_cp = cp;
                rm_len = dp->rec_len;
                printf("[DEBUG] in rm_child(): rm: [%d %s]", dp->rec_len, temp);
            } else
                printf("[%d %s] ", dp->rec_len, temp);


            last_len = dp->rec_len;
            cp += dp->rec_len;
            dp = (DIR*)cp;
            j++; // get count of entries into j 
        }

        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;

        printf("[DEBUG] in rm_child(): [%d %s]\n", dp->rec_len, temp);
        printf("[DEBUG] in rm_child(): block_i: %d\n", block_i);

        if (j == 0) { // first entry
            printf("[DEBUG] in rm_child(): First entry\n");

            printf("[DEBUG] in rm_child(): deallocating data block: %d\n", block_i);
            bdalloc(pmip->dev, pmip->inode.i_block[block_i]); // dealloc this block

            for (i=block_i; i<pmip->inode.i_blocks; i++); // move up blocks
        } else if (i==0) { // last entry
            cp -= last_len;
            last_len = dp->rec_len;
            dp = (DIR*)cp;
            dp->rec_len += last_len;
            printf("[DEBUG] in rm_child(): dp->rec_len: %d\n", dp->rec_len);
        } else { // middle entry
            size = buf+BLKSIZE - (rm_cp+rm_len);
            printf("[DEBUG] in rm_child(): Copying %d bytes\n", size);
            memmove(rm_cp, rm_cp + rm_len, size);
            cp -= rm_len; 
            dp = (DIR*)cp;

            dp->rec_len += rm_len;

            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
        }

        put_block(pmip->dev, pmip->inode.i_block[block_i], buf);
    }

    return 0;
}
