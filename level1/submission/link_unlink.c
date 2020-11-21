#include "globals.h"

#include <time.h>
#include <stdio.h>

void link() 
{
    int ino = getino(pathname); 
    if(!ino) 
    {
        printf("Source file does not exist\n");
        return;
    }
    MINODE * source = iget(dev, ino); 
    if(S_ISDIR(source->inode.i_mode)) 
    {
        printf("Link to directory not allowed\n");
        return;
    }
    dbname(pathname2);
    int dirino = getino(dname);
    if(!dirino) 
    {
        printf("Destination directory does not exist\n");
        return;
    }
    MINODE * dirnode = iget(dev, dirino);
    if(!S_ISDIR(dirnode->inode.i_mode)) 
    {
        printf("Destination is not a directory\n");
        return;
    }
    char *base = strdup(bname);
    if(search(dirnode, base)) 
    {
        printf("Destination file already exists\n");
        return;
    }

    free(base); 
    enter_name(dirnode, ino, bname); 

    source->inode.i_links_count++; 
    source->dirty = 1; 

    iput(source); 
    iput(dirnode); 
}

void unlink() 
{
    int ino = getino(pathname); 
    if(!ino) 
    {
        printf("Could not find specified path\n");
        return;
    }
    MINODE * m = iget(dev, ino); 
    if(S_ISDIR(m->inode.i_mode)) 
    {
        printf("Specified path is a directory\n");
        return;
    }
    if(--(m->inode.i_links_count) == 0) 
    {
        mytruncate(m); 
        idalloc(m->dev, m->ino);
    }
    else
    {
        m->dirty = 1; 
    }

   
    dbname(pathname);
    MINODE * parent = iget(dev, getino(dname)); 
    rmchild(parent, bname);

    iput(parent); 
    iput(m); 
}