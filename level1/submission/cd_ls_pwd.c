#include "globals.h"
#include "util.h"

#include <time.h>
#include <stdio.h>
#include <string.h>

void mychdir() {
    if (strlen(pathname) == 0 || strcmp(pathname, "/") == 0) {
        iput(running->cwd);  
        running->cwd = root; 
        return;
    } else {
        char temp[256];
        strcpy(temp, pathname); 
        
        if (pathname[0] == '/') 
            dev = root->dev;
        else
            dev = running->cwd->dev;
        
        int ino = getino(temp);            
        MINODE *mip = iget(dev, ino);      
        
        if (!(S_ISDIR(mip->inode.i_mode))) {
            printf("%s is not a directory", pathname);
            return;
        }

        iput(running->cwd); 
        running->cwd = mip; 
    }
}

void ls_file(int ino, char *filename) {
    MINODE *mip = iget(dev, ino); 
    const char *t1 = "xwrxwrxwr-------";
    const char *t2 = "----------------";

    if (S_ISREG(mip->inode.i_mode)) 
        putchar('-');
    else if (S_ISDIR(mip->inode.i_mode)) 
        putchar('d');                    
    else if (S_ISLNK(mip->inode.i_mode)) 
        putchar('l');                    

    for (int i = 8; i >= 0; i--)
        putchar(((mip->inode.i_mode) & (1 << i)) ? t1[i] : t2[i]); 

    time_t t = (time_t)(mip->inode.i_ctime);
    char temp[256];
    strcpy(temp, ctime(&t))[strlen(temp) - 1] = '\0';

    printf(" %4d %4d %4d %4d %s %s", 
            (int) (mip->inode.i_links_count), 
            (int) (mip->inode.i_gid),
            (int) (mip->inode.i_uid), 
            (int) (mip->inode.i_size),
            temp,
            filename);

    if (S_ISLNK(mip->inode.i_mode))                     
        printf(" -> %s", (char *)(mip->inode.i_block)); 

    putchar('\n');

    iput(mip);
}

void ls_dir(char *dirname) {
    int ino = getino(dirname);    
    MINODE *mip = iget(dev, ino); 
    char buf[BLKSIZE];

    for (int i = 0; i < 12 && mip->inode.i_block[i]; i++) {
        get_block(dev, mip->inode.i_block[i], buf); 
        char *cp = buf;
        DIR *dp = (DIR *)buf; 

        while (cp < BLKSIZE + buf) {
            char temp[256];                        
            strncpy(temp, dp->name, dp->name_len); 
            temp[dp->name_len] = 0;                
            ls_file(dp->inode, temp);              
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }

    iput(mip); 
}

void ls() {
    char temp[256];
    int ino;
    if (strlen(pathname) == 0) {
        strcpy(pathname, ".");
        ino = running->cwd->ino;
    } else {
        strcpy(temp, pathname); 
        ino = getino(temp);     
    }

    MINODE *mip = iget(dev, ino); 

    if (S_ISDIR(mip->inode.i_mode))
        ls_dir(pathname); 
    else
        ls_file(ino, pathname);

    iput(mip); 
}

void rpwd(MINODE *wd) {
    char buf[256];
    if (wd == root) 
        return;

    int parent_ino = search(wd, ".."); 
    int my_ino = wd->ino;
    MINODE *pip = iget(dev, parent_ino); 
    findmyname(pip, my_ino, buf);
    rpwd(pip); 
    iput(pip); 
    printf("/%s", buf);
}

void pwd() {
    if (running->cwd == root) {
        puts("/"); 
        return;
    }

    rpwd(running->cwd); 
    putchar('\n');
}