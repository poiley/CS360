#include "globals.h"
#include "util.h"

#include <stdio.h>

int rmchild(MINODE *pip, char *name) {
    char *cp, buf[BLKSIZE];
    DIR *dp, *prev;
    int current;

    for (int i = 0; i < 12 && pip->inode.i_block[i]; i++) {
        current = 0;

        get_block(dev, pip->inode.i_block[i], buf);
        cp = buf;
        dp = (DIR *)buf;
        prev = 0;
        while (cp < buf + BLKSIZE) {
            if (strncmp(dp->name, name, dp->name_len) == 0) {
                int ideal = ideal_length(dp->name_len);
                if (ideal != dp->rec_len) {
                    prev->rec_len += dp->rec_len;
                } else if (prev == NULL && cp + dp->rec_len == buf + 1024) {
                    char empty[BLKSIZE] = {0};
                    put_block(dev, pip->inode.i_block[i], empty);
                    bdalloc(dev, pip->inode.i_block[i]); 
                    pip->inode.i_size -= BLKSIZE;        
                    
                    for (int j = i; j < 11; j++)
                        pip->inode.i_block[j] = pip->inode.i_block[j + 1];

                    pip->inode.i_block[11] = 0;
                } else {
                    int removed = dp->rec_len;
                    char *temp = buf;
                    DIR *last = (DIR *)temp;

                    while (temp + last->rec_len < buf + BLKSIZE) {
                        temp += last->rec_len;
                        last = (DIR *)temp;
                    }

                    last->rec_len = last->rec_len + removed;

                    memcpy(cp, cp + removed, BLKSIZE - current - removed + 1);
                }

                put_block(dev, pip->inode.i_block[i], buf);
                pip->dirty = 1;
                return 1;
            }

            cp += dp->rec_len; 
            current += dp->rec_len;
            prev = dp;
            dp = (DIR *)cp;
        }

        return 0;
    }

    printf("ERROR: %s does not exist.\n", bname);
    return 1;
}

void rmdir() {
    dbname(pathname);
    char *temp = strdup(pathname);
    int ino = getino(temp); 
    free(temp);
    if (!ino) {
        printf("ERROR: %s does not exist.\n", pathname);
        return;
    }

    MINODE *mip = iget(dev, ino);                              
    if (running->uid != mip->inode.i_uid && running->uid != 0) {
        printf("ERROR: You do not have permission to remove %s.\n", pathname);
        iput(mip); 
        return;
    }

    if (!S_ISDIR(mip->inode.i_mode)) {
        printf("ERROR: %s is not a directory", pathname);
        iput(mip); 
        return;
    }

    if (mip->inode.i_links_count > 2) {
        printf("ERROR: Directory %s is not empty.\n", pathname);
        iput(mip); 
        return;
    }

    if (mip->inode.i_links_count == 2) {
        char buf[BLKSIZE], *cp;
        DIR *dp;
        get_block(dev, mip->inode.i_block[0], buf);
        cp = buf;
        dp = (DIR *)buf;
        cp += dp->rec_len;
        dp = (DIR *)cp;          
        if (dp->rec_len != 1012) {
            printf("ERROR: Directory %s is not empty.\n", pathname);
            iput(mip); 
            return;
        }
    }

    for (int i = 0; i < 12; i++) {
        if (mip->inode.i_block[i] == 0)
            continue;
        bdalloc(mip->dev, mip->inode.i_block[i]); 
    }

    idalloc(mip->dev, mip->ino);                              
                                 
    iput(mip);                   

    MINODE *pip = iget(dev, getino(dname)); 
    rmchild(pip, bname);

    pip->inode.i_links_count--; 
    pip->inode.i_atime = pip->inode.i_mtime = time(0L);
    pip->dirty = 1; 
    iput(pip);      
}