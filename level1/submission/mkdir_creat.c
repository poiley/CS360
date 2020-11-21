#include "globals.h"

#include <time.h>
#include <stdio.h>


void enter_name(MINODE * pip, int myino, char * myname) 
{
    int need_len = ideal_length(strlen(myname)); 
    for(int i = 0; i < 12; i++) 
    {
        char buf[BLKSIZE];
        DIR * dp = (DIR*)buf;
        char * cp = buf;

        if(pip->inode.i_block[i] == 0) 
        {
           
            pip->inode.i_block[i] = balloc(dev);
            get_block(pip->dev, pip->inode.i_block[i], buf);
            *dp = (DIR){.inode = myino, .rec_len = BLKSIZE, .name_len = strlen(myname)};
            strncpy(dp->name, myname, dp->name_len);
            return;
        }
        
        get_block(pip->dev, pip->inode.i_block[i], buf); 

       
        while(cp + dp->rec_len < buf + BLKSIZE)
        {
            cp += dp->rec_len;
            dp = (DIR*)cp;
        }
        

        int remain = dp->rec_len - ideal_length(dp->name_len); 
                                                                
        if(remain >= need_len)
        {
           
            dp->rec_len = ideal_length(dp->name_len);
            cp += dp->rec_len;
            dp = (DIR*)cp;
            *dp = (DIR){.inode = myino, .rec_len = remain, .name_len = strlen(myname)};
            strncpy(dp->name, myname, dp->name_len);
            put_block(pip->dev, pip->inode.i_block[i], buf);
            pip->dirty = 1;
            return;
        }
    }
}

int make_entry(int dir, char * name)
{
    dbname(name); 
    int parent = getino(dname);  
    if(!parent) 
    {
        printf("ERROR: Specified path does not exist\n");
        return 0;
    }
    MINODE * pip = iget(dev, parent);  
    if(!S_ISDIR(pip->inode.i_mode)) 
    {
        printf("ERROR: Specified path is not a directory\n");
        return 0;
    }
    if(search(pip, bname)) 
    {
        printf("ERROR: Entry %s already exists\n", bname);
        return 0;
    }

    if(dir)
        pip->inode.i_links_count++; 
    
    int ino = ialloc(dev), bno = dir ? balloc(dev) : 0; 

    MINODE * mip = iget(dev, ino); 
    INODE * ip = &(mip->inode); 
    time_t t = time(0L);
    *ip = (INODE){.i_mode = (dir ? 0x41ED : 0x81A4), .i_uid = running->uid, .i_gid = running->gid, .i_size = (dir ? BLKSIZE : 0), .i_links_count = (dir ? 2 : 1),
            .i_atime = t, .i_ctime = t, .i_mtime = t, .i_blocks = (dir ? 2 : 0), .i_block = {bno}};
    mip->dirty = 1; 

    if(dir)
    {
        char buf[BLKSIZE] = {0}; 
        char * cp = buf;
        DIR * dp = (DIR*)buf; 
        *dp = (DIR){.inode = ino, .rec_len = 12, .name_len = 1, .name = "."};
        cp += dp->rec_len;
        dp = (DIR*)cp;
        *dp = (DIR){.inode = pip->ino, .rec_len = BLKSIZE - 12, .name_len = 2, .name = ".."};
        put_block(dev, bno, buf);
    }

    enter_name(pip, ino, bname); 
                                

    iput(mip); 
    iput(pip); 

    return ino;
}

void makedir()
{
    make_entry(1, pathname); 
}

void create_file() 
{
    make_entry(0, pathname); 
}
