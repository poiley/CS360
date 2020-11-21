#include "globals.h"
#include "util.h"


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




