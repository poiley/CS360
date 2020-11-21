#include "level1.h"
#include "globals.h"
#include "util.h"

#include <time.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>



void mychdir() 
{
    if (strlen(pathname) == 0 || strcmp(pathname, "/") == 0) 
    {
        iput(running->cwd); 
        running->cwd = root; 
        return;
    }
    else
    {
        char temp[256];
        strcpy(temp,pathname); 
        if(pathname[0] == '/') 
        {
            dev = root->dev;
        }
        else
            dev = running->cwd->dev;
        int ino = getino(temp); 
        MINODE *mip = iget(dev, ino); 
        if(!(S_ISDIR(mip->inode.i_mode))) 
        {
            printf("%s is not a directory", pathname);
            return;
        }
        iput(running->cwd); 
        running->cwd = mip; 
    }
}

void ls_file(int ino, char *filename) 

{
    MINODE * mip = iget(dev, ino); 
    const char * t1 = "xwrxwrxwr-------";
    const char * t2 = "----------------";
    if(S_ISREG(mip->inode.i_mode)) 
        putchar('-');
    else if(S_ISDIR(mip->inode.i_mode)) 
        putchar('d'); 
    else if(S_ISLNK(mip->inode.i_mode)) 
        putchar('l'); 
    for(int i = 8; i >= 0; i--)
        putchar(((mip->inode.i_mode) & (1 << i)) ? t1[i] : t2[i]); 

    time_t t = (time_t)(mip->inode.i_ctime);
    char temp[256];
    strcpy(temp, ctime(&t))[strlen(temp) - 1] = '\0';

    printf(" %4d %4d %4d %4d %s %s", (int)(mip->inode.i_links_count), (int)(mip->inode.i_gid), 
    (int)(mip->inode.i_uid), (int)(mip->inode.i_size),
            temp, filename);

    if(S_ISLNK(mip->inode.i_mode)) 
        printf(" -> %s", (char*)(mip->inode.i_block)); 

    putchar('\n');

    iput(mip);   
}

void ls_dir(char * dirname)	
                            
{
    int ino = getino(dirname); 
    MINODE * mip = iget(dev, ino);	
    char buf[BLKSIZE];
    for(int i = 0; i < 12 && mip->inode.i_block[i]; i++)   
    {
        get_block(dev, mip->inode.i_block[i], buf);  
        char * cp = buf;
        DIR * dp = (DIR*)buf;		

        while(cp < BLKSIZE + buf) 
        {
            char temp[256]; 
            strncpy(temp, dp->name, dp->name_len); 
            temp[dp->name_len] = 0; 
            ls_file(dp->inode, temp); 
            cp += dp->rec_len;
            dp = (DIR*)cp;
        }
    }
    iput(mip); 
}

void ls() 
{
    char temp[256];
    int ino;
    if(strlen(pathname) == 0) 
    {
        strcpy(pathname, ".");
        ino = running->cwd->ino; 
    }
    else 
    {
        strcpy(temp, pathname); 
        ino = getino(temp); 
    }
    MINODE *mip = iget(dev, ino); 

    if(S_ISDIR(mip->inode.i_mode)) 
    {
        ls_dir(pathname); 
    }
    else
    {
        ls_file(ino, pathname); 
    }
    iput(mip); 
}

void rpwd(MINODE *wd) 
{
    char buf[256];
    if (wd==root) 
        return;
    int parent_ino = search(wd,".."); 
    int my_ino = wd->ino;
    MINODE * pip = iget(dev, parent_ino); 
    findmyname(pip, my_ino, buf);
    rpwd(pip); 
    iput(pip); 
    printf("/%s", buf);
}

void pwd() 
{
    if(running->cwd == root) 
    {
        puts("/"); 
        return;
    }
    rpwd(running->cwd); 
    putchar('\n');
}

int ideal_length(int name_len) 
{
    return 4 * ((8 + name_len + 3) / 4); 
}


int rmchild(MINODE * pip, char * name) 
{
    char *cp, buf[BLKSIZE];
    DIR *dp, *prev;
    int current;

    for(int i = 0; i < 12 && pip->inode.i_block[i]; i++)
    {
        current = 0;

        get_block(dev, pip->inode.i_block[i], buf);
        cp = buf;
        dp = (DIR *) buf;
        prev = 0;
        while (cp < buf + BLKSIZE)
        {
            if (strncmp(dp->name, name, dp->name_len) == 0)
            {
                int ideal = ideal_length(dp->name_len);
                if(ideal != dp->rec_len) 
                {
                    
                    prev->rec_len += dp->rec_len;
                }
                else if(prev == NULL && cp + dp->rec_len == buf + 1024)
                                                                
                {
                   
                    char empty[BLKSIZE] = {0};
                    put_block(dev, pip->inode.i_block[i], empty);
                    bdalloc(dev, pip ->inode.i_block[i]); 
                    pip->inode.i_size -= BLKSIZE; 
                    for(int j = i; j < 11; j++)
                    {
                        pip->inode.i_block[j] = pip->inode.i_block[j+1];
                    }
                    pip->inode.i_block[11] = 0;
                }
                else 
                {
                   
                    int removed = dp->rec_len;
                    char *temp = buf;
                    DIR *last = (DIR *)temp;

                    while(temp + last->rec_len < buf + BLKSIZE) 
                    {
                        temp += last->rec_len;
                        last = (DIR *) temp;
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
            dp = (DIR *) cp;
        }
        return 0;
    }

    printf("ERROR: %s does not exist.\n",bname);
    return 1;
}

void rmdir() 
{
    dbname(pathname);
    char *temp = strdup(pathname);
    int ino = getino(temp); 
    free(temp);
    if(!ino) 
    {
        printf("ERROR: %s does not exist.\n", pathname);
        return;
    }
    MINODE * mip = iget(dev, ino); 
    if(running->uid != mip->inode.i_uid && running->uid != 0) 
    {
        printf("ERROR: You do not have permission to remove %s.\n", pathname);
        iput(mip); 
        return;
    }
    if(!S_ISDIR(mip->inode.i_mode)) 
    {
        printf("ERROR: %s is not a directory", pathname);
        iput(mip); 
        return;
    }
    if(mip->inode.i_links_count > 2) 
    {
        printf("ERROR: Directory %s is not empty.\n", pathname);
        iput(mip); 
        return;
    }
    if(mip->inode.i_links_count == 2) 
    {
        char buf[BLKSIZE], * cp;
        DIR *dp;
        get_block(dev, mip->inode.i_block[0],buf);
        cp = buf;
        dp = (DIR *) buf;
        cp += dp->rec_len;
        dp = (DIR *) cp; 
        if(dp->rec_len != 1012) 
        {
            printf("ERROR: Directory %s is not empty.\n", pathname);
            iput(mip); 
            return;
        }
    }

    for(int i = 0; i < 12; i++) 
    {
        if (mip->inode.i_block[i] == 0)
            continue;
        bdalloc(mip->dev,mip->inode.i_block[i]); 
    }
    idalloc(mip->dev, mip->ino); 
                                 
                                 
    iput(mip); 
    
    MINODE * pip = iget(dev, getino(dname)); 
    rmchild(pip, bname);

    pip->inode.i_links_count--; 
    pip->inode.i_atime = pip->inode.i_mtime = time(0L);
    pip->dirty = 1; 
    iput(pip); 
}



void symlink() 
{
    char * pathname_dup = strdup(pathname); 
    if(!getino(pathname_dup)) 
    {
        free(pathname_dup); 
        printf("Specified source path does not exist\n");
        return;
    }
    free(pathname_dup); 
    int ino = make_entry(0, pathname2); 
    MINODE * m = iget(dev, ino); 
    m->inode.i_mode = 0xA1A4; 
    strcpy((char*)(m->inode.i_block), pathname);

    iput(m);
}

void readlink() 
{
    int ino = getino(pathname); 
    if(!ino) 
    {
        printf("Specified path does not exist\n");
        return;
    }
    MINODE * m = iget(dev, ino);
    if(!S_ISLNK(m->inode.i_mode)) 
    {
        printf("Specified path is not a link file\n");
        return;
    }
    printf("%s\n", (char*)(m->inode.i_block));

    iput(m);
}

void mystat()
{
    int ino = getino(pathname); 
    if(!ino) 
    {
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

void mychmod()
{
    int ino = getino(pathname); 
    if(!ino) 
    {
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

void touch(char *name) 
{
	int ino = getino(name);
    if(ino == -1) 
	{ 
		printf("not a path\n");
		return; 
	}
	MINODE *mip = iget(dev, ino); 
	mip->inode.i_atime = mip->inode.i_mtime = time(0L); 
	mip->dirty = 1;

	iput(mip);

	return;
} 

void quit()
{
   
    for (int i = 0; i < NMINODE; i++)
        if (minode[i].refCount > 0 && minode[i].dirty == 1)
        {
            minode[i].refCount = 1;
            iput(&(minode[i]));
        }
    exit(0); 
}




