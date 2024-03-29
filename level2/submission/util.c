#include "globals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
// #include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>


int get_block(int dev, int blk, char *buf) {
    lseek(dev, blk*BLKSIZE, 0); 
    return (read(dev, buf, BLKSIZE) < 0) ? 0 : 1; 
}   

int put_block(int dev, int blk, char *buf) {
    lseek(dev, blk*BLKSIZE, 0);
    return (write(dev, buf, BLKSIZE) != BLKSIZE) ? 0 : 1;
}   

int tokenize(char *pathname) {
    char * temp;
    int i = 0;
    temp = strtok(pathname, "/");
    
    do
        name[i++] = temp;  
    while((temp = strtok(NULL, "/")) && i < 64); 
    
    return i;
}

MINODE *iget(int dev, int ino) {
    for(int i = 0; i < NMINODE; i++) 
        if(minode[i].refCount > 0 && minode[i].dev == dev && minode[i].ino == ino) { 
            minode[i].refCount++;	
            return (minode + i); 	
        }

    int i;
    for(i = 0; i < NMINODE; i++) 
        if(minode[i].refCount == 0) {
            minode[i] = (MINODE){.refCount = 1, .dev = dev, .ino = ino};
            break;
        }

    char buf[BLKSIZE];
    int blk = (ino - 1) / 8 + inode_start, offset = (ino - 1) % 8; 
    get_block(dev, blk, buf); 
    minode[i].inode = *((INODE*)buf + offset); 
    return minode + i;	
}

int iput(MINODE *mip) {
    if (--(mip->refCount) == 0 || mip->dirty) {
        char buf[BLKSIZE];
        int blk = (mip->ino - 1) / 8 + inode_start, offset = (mip->ino - 1) % 8; 
        get_block(mip->dev, blk, buf); 
        *((INODE*)buf + offset) = mip->inode; 
        put_block(mip->dev, blk, buf); 
    }
}

int search(MINODE *mip, char *name) {
    char dbuf[BLKSIZE], temp[256];
    DIR *dp;
    char * cp;
    for (int i = 0; i < 12 && mip->inode.i_block[i]; i++) {
        get_block(mip->dev, mip->inode.i_block[i], dbuf);
        dp = (DIR*) dbuf;
        cp = dbuf;			

        while (cp < dbuf + BLKSIZE) {
            strncpy(temp, dp->name, dp->name_len);	
            temp[dp->name_len] = 0;		
        
            if(strcmp(name,dp->name) == 0)	 
                return dp->inode;

            cp += dp->rec_len; 	
            dp = (DIR*)cp;
        }
    }

    return 0; 
}

int getino(char *pathname) { 
    MINODE * cur = (pathname[0] == '/') ? root : running->cwd;  
    cur->refCount++;
   
    int size = tokenize(pathname);  
    int ino;

    if(!name[0])		
        ino = cur->ino;
    else
        for(int i = 0; i < size; i++) {
            ino = search(cur, name[i]);  
            if(ino == 0)
                return 0;
            iput(cur);
            cur = iget(cur->dev, ino);
        }
    
    iput(cur);
    return ino; 
}

int findmyname(MINODE *parent, u32 myino, char *myname) {	
    char dbuf[BLKSIZE], temp[256];
    for(int i = 0; i < 12 && parent->inode.i_block[i]; i++) {
        get_block(parent->dev, parent->inode.i_block[i], dbuf); 
        char * cp = dbuf;
        DIR * dp = (DIR*)dbuf;
 
        while(cp < dbuf + BLKSIZE) {
            if(dp->inode == myino) {
                strncpy(myname, dp->name, dp->name_len);   
                return 1;
            }
            cp += dp->rec_len; 
            dp = (DIR*)cp;
        }
    }
    return 0;
}


int tst_bit(char * buf, int bit) { return buf[bit/8] & (1 << (bit%8)); }

void set_bit(char * buf, int bit) { buf[bit/8] |= (1 << (bit%8)); }

void clr_bit(char * buf, int bit) { buf[bit/8] &= ~(1 << (bit%8)); }

int decFreeInodes(int dev) {
    char buf[BLKSIZE];
    
    get_block(dev, 1, buf);
    ((SUPER*)buf)->s_free_inodes_count--;   
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    ((GD*)buf)->bg_free_inodes_count--;   
    put_block(dev, 2, buf);
}

int incFreeInodes(int dev) {
    char buf[BLKSIZE];
    
    get_block(dev, 1, buf);
    ((SUPER *)buf)->s_free_inodes_count++; 
    put_block(dev, 1, buf);			

    get_block(dev, 2, buf);
    ((GD *)buf)->bg_free_inodes_count++;  
    put_block(dev, 2, buf);		
}


int ialloc(int dev) {
    char buf[BLKSIZE];
    get_block(dev, imap, buf);   
    for(int i = 0; i < ninodes; i++)
        if(tst_bit(buf, i) == 0) {
            set_bit(buf, i);	
            decFreeInodes(dev);	
            put_block(dev, imap, buf);
            return i + 1;
        }
    return 0;
}


int balloc(int dev) {
    char buf[BLKSIZE];

    get_block(dev, bmap, buf);
    for(int i = 0; i < nblocks; i++) {
        if(tst_bit(buf, i) == 0) {
            set_bit(buf, i);	
            decFreeInodes(dev);
            put_block(dev, bmap, buf);	
            return i + 1;		
        }
    }
        
    return 0;
}


void idalloc(int dev, int ino) {
    char buf[BLKSIZE];
    get_block(dev, imap, buf);

    if(ino > ninodes) {
        printf("ERROR: inumber %d out of range.\n", ino);
        return;
    }

    get_block(dev,imap,buf);	
    clr_bit(buf,ino-1);		
    put_block(dev,imap,buf);	
    incFreeInodes(dev);		
}


void bdalloc(int dev, int bno) {
    char buf[BLKSIZE];
    
    if(bno > nblocks) {
        printf("ERROR: bnumber %d out of range.\n", bno);
        return;
    }

    get_block(dev,bmap,buf); 
    clr_bit(buf,bno-1);		
    put_block(dev,bmap,buf);	
    incFreeInodes(dev);		
}

void mytruncate(MINODE * mip) {
    for(int i = 0; i < 12 && mip->inode.i_block[i]; i++)
        bdalloc(mip->dev, mip->inode.i_block[i]); 
}

void dbname(char *pathname) {
    char temp[256];
    strcpy(temp, pathname);	
    strcpy(dname, dirname(temp));	
    strcpy(temp, pathname);
    strcpy(bname, basename(temp));	
}

void zero_block(int dev, int blk) {
    char buf[BLKSIZE] = {0};
    put_block(dev, blk, buf);	
}

void mystat() {
    int ino = getino(pathname); 
    if(!ino) {
        printf("File not found\n");
        return;
    }

    MINODE * m = iget(dev, ino); 
    dbname(pathname); 
    printf("File: %s\n", bname);
    printf("Size: %d\n", m->inode.i_size);
    printf("Blocks: %d\n", m->inode.i_blocks);

    char * type;
   
    if(S_ISDIR(m->inode.i_mode)) 
        type = "Dir";
    else if(S_ISLNK(m->inode.i_mode)) 
        type = "Link";
    else 
        type = "Reg";

    time_t a = (time_t) m->inode.i_atime, c = (time_t) m->inode.i_ctime, mod = (time_t) m->inode.i_atime;

    printf("Type: %s\n", type);
    printf("Inode: %d\n", ino);
    printf("Links: %d\n", m->inode.i_links_count);
    printf("Access Time: %s\n", ctime(&a));
    printf("Modify Time: %s\n", ctime(&mod));
    printf("Change Time: %s\n", ctime(&c));
    printf("Device: %d\n", m->dev);
    printf("UID: %d\n", m->inode.i_uid);
    printf("GID: %d\n", m->inode.i_gid);

    iput(m);
}

void mychmod() {
    int ino = getino(pathname); 
    if(!ino) {
        printf("File does not exist\n");
        return;
    }
   
    MINODE * m = iget(dev, ino); 
    int new;
    sscanf(pathname2, "%o", &new);
    
    int old = m->inode.i_mode;
    old >>= 9;  
    new |= (old << 9);

    m->inode.i_mode = new;
    m->dirty = 1; 

    iput(m);
}

void touch(char *name) {
	int ino = getino(name);
    if(ino == -1) { 
		printf("not a path\n");
		return; 
	}

	MINODE *mip = iget(dev, ino); 
	mip->inode.i_atime = mip->inode.i_mtime = time(0L); 
	mip->dirty = 1;

	iput(mip);

	return;
} 

void quit() {
    for (int i = 0; i < NMINODE; i++)
        if (minode[i].refCount > 0 && minode[i].dirty == 1) {
            minode[i].refCount = 1;
            iput(&(minode[i]));
        }
        
    exit(0); 
}