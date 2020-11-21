#include "level1.h"
#include "globals.h"
#include "util.h"

#include <time.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

/*each inode has a unique inode number, ino = 1, 2 ... */

void mychdir() // cd function that allows users to change the directory they are in 
{
    if (strlen(pathname) == 0 || strcmp(pathname, "/") == 0) // if no pathname given or path is root 
    {
        iput(running->cwd); // release old DIR node
        running->cwd = root; // change current working directory to the root
        return;
    }
    else
    {
        char temp[256];
        strcpy(temp,pathname); // make temporary copy of the pathname 
        if(pathname[0] == '/') // if pathname starts from root 
        {
            dev = root->dev;
        }
        else
            dev = running->cwd->dev;
        int ino = getino(temp); // get inode number of pathname 
        MINODE *mip = iget(dev, ino); // get new DIR inode
        if(!(S_ISDIR(mip->inode.i_mode))) // if not a directory (and we therefore can't cd into it)
        {
            printf("%s is not a directory", pathname);
            return;
        }
        iput(running->cwd); // iput (release) old DIR node
        running->cwd = mip; // change cwd to mip
    }
}

void ls_file(int ino, char *filename) // closesly adapted from function in Professor Wang's book
// STAT filename; do ls to get file information in a STAT structure
{
    MINODE * mip = iget(dev, ino); // get new  node
    const char * t1 = "xwrxwrxwr-------";
    const char * t2 = "----------------";
    if(S_ISREG(mip->inode.i_mode)) // if it's a regular file
        putchar('-');
    else if(S_ISDIR(mip->inode.i_mode)) // if it's a directory
        putchar('d'); // indicate this with 'd'
    else if(S_ISLNK(mip->inode.i_mode)) // if it's a link'
        putchar('l'); // indicate this withan l
    for(int i = 8; i >= 0; i--)
        putchar(((mip->inode.i_mode) & (1 << i)) ? t1[i] : t2[i]); // print appropriate permissions

    time_t t = (time_t)(mip->inode.i_ctime);
    char temp[256];
    strcpy(temp, ctime(&t))[strlen(temp) - 1] = '\0';

    printf(" %4d %4d %4d %4d %s %s", (int)(mip->inode.i_links_count), (int)(mip->inode.i_gid), 
    (int)(mip->inode.i_uid), (int)(mip->inode.i_size),
            temp, filename);

    if(S_ISLNK(mip->inode.i_mode)) // if it is a link
        printf(" -> %s", (char*)(mip->inode.i_block)); // show the linked files when printing

    putchar('\n');

    iput(mip);   
}

void ls_dir(char * dirname)	// use opendir() and readir() to get filenames under dirname; For 
                            // each filename: call 
{
    int ino = getino(dirname); // get inode number of pathname
    MINODE * mip = iget(dev, ino);	// mip points at dirname's minode, INODE, other fields
    char buf[BLKSIZE];
    for(int i = 0; i < 12 && mip->inode.i_block[i]; i++)   // step through each DIR entry
    {
        get_block(dev, mip->inode.i_block[i], buf);  // get next DIR entry for blocks
        char * cp = buf;
        DIR * dp = (DIR*)buf;		// create a directory for block retrieved

        while(cp < BLKSIZE + buf) // while cp is smaller than block size and buf
        {
            char temp[256]; // create temp string
            strncpy(temp, dp->name, dp->name_len); // copy dp->name to temp string
            temp[dp->name_len] = 0; // last char = 0
            ls_file(dp->inode, temp); // now run lsfile using this temp
            cp += dp->rec_len;
            dp = (DIR*)cp;
        }
    }
    iput(mip); // release mip
}

void ls() // also adapted from Professor Wang's function in his book
{
    char temp[256];
    int ino;
    if(strlen(pathname) == 0) // if no specified pathname
    {
        strcpy(pathname, ".");
        ino = running->cwd->ino; 
    }
    else // if given a pathname to ls into
    {
        strcpy(temp, pathname); // create temporary copy
        ino = getino(temp); // get inode number of pathname
    }
    MINODE *mip = iget(dev, ino); // get inode

    if(S_ISDIR(mip->inode.i_mode)) // if it is a directory
    {
        ls_dir(pathname); // call ls function for directories
    }
    else
    {
        ls_file(ino, pathname); // call ls function for file
    }
    iput(mip); // release mip
}

void rpwd(MINODE *wd) // recursive calls 
{
    char buf[256];
    if (wd==root) // if working directory is root (base case)
        return;
    int parent_ino = search(wd,".."); // search wd for entry with given name .. 
    int my_ino = wd->ino;
    MINODE * pip = iget(dev, parent_ino); // get new inode 
    findmyname(pip, my_ino, buf);
    rpwd(pip); // recursive call to rpwd with parent
    iput(pip); // release
    printf("/%s", buf);
}

void pwd() // pathname of current working directory
{
    if(running->cwd == root) // if we are simply at the root
    {
        puts("/"); // just print / for root
        return;
    }
    rpwd(running->cwd); // call to rpwd
    putchar('\n');
}

int ideal_length(int name_len) // function equation taken from Professor Wang's book
{
    return 4 * ((8 + name_len + 3) / 4); // a multiple of 4
}

void enter_name(MINODE * pip, int myino, char * myname) // Adapted from Professor Wang's algorithm in his book 
{
    int need_len = ideal_length(strlen(myname)); // get the ideal length 
    for(int i = 0; i < 12; i++) // for each data block of parent DIR do (assume: only 12 direct blocks)
    {
        char buf[BLKSIZE];
        DIR * dp = (DIR*)buf;
        char * cp = buf;

        if(pip->inode.i_block[i] == 0) // if no space in existing data block(s)
        {
            /* allocate a new block: Allocate a new data block; increment parent size by BLKSIZE.
            Enter new entry as the first entry in the new data block with rec_len = BLKSIZE. */
            pip->inode.i_block[i] = balloc(dev);
            get_block(pip->dev, pip->inode.i_block[i], buf);
            *dp = (DIR){.inode = myino, .rec_len = BLKSIZE, .name_len = strlen(myname)};
            strncpy(dp->name, myname, dp->name_len);
            return;
        }
        
        get_block(pip->dev, pip->inode.i_block[i], buf); // Get parent’s data block into a buf[ ];

        /*get to the last entry in the block*/
        while(cp + dp->rec_len < buf + BLKSIZE)
        {
            cp += dp->rec_len;
            dp = (DIR*)cp;
        }
        // dp now points at last entry in block

        int remain = dp->rec_len - ideal_length(dp->name_len); // remain is LAST entry’s rec_len - 
                                                                // its ideal_length;
        if(remain >= need_len)
        {
            /*enter the new entry as the LAST entry and trim the previous entry rec_len to its ideal_length*/
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

//called in makedir 
//dir = 1 = directory
//dir = 0 = file
int make_entry(int dir, char * name) /* Adapted from Professor Wang's algorithm in his book */
{
    dbname(name); //new database
    int parent = getino(dname);  // get inode number 
    if(!parent) // check if path exists or not 
    {
        printf("ERROR: Specified path does not exist\n");
        return 0;
    }
    MINODE * pip = iget(dev, parent);  // get new inode
    if(!S_ISDIR(pip->inode.i_mode)) // check if directory or not
    {
        printf("ERROR: Specified path is not a directory\n");
        return 0;
    }
    if(search(pip, bname)) // check to see if the entry already exists
    {
        printf("ERROR: Entry %s already exists\n", bname);
        return 0;
    }

    if(dir)
        pip->inode.i_links_count++; // increment parent INODE’s links_count by 1
    
    int ino = ialloc(dev), bno = dir ? balloc(dev) : 0; // allocate and inode and a disk block

    MINODE * mip = iget(dev, ino); // load inode into minode 
    INODE * ip = &(mip->inode); // initialize mip->INODE as a DIR INODE;
    time_t t = time(0L);
    *ip = (INODE){.i_mode = (dir ? 0x41ED : 0x81A4), .i_uid = running->uid, .i_gid = running->gid, .i_size = (dir ? BLKSIZE : 0), .i_links_count = (dir ? 2 : 1),
            .i_atime = t, .i_ctime = t, .i_mtime = t, .i_blocks = (dir ? 2 : 0), .i_block = {bno}};
    mip->dirty = 1; // mark minode modified (dirty);

    if(dir)
    {
        char buf[BLKSIZE] = {0}; // create buffer with blksize
        char * cp = buf;
        DIR * dp = (DIR*)buf; // create DIR from buf
        *dp = (DIR){.inode = ino, .rec_len = 12, .name_len = 1, .name = "."};
        cp += dp->rec_len;
        dp = (DIR*)cp;
        *dp = (DIR){.inode = pip->ino, .rec_len = BLKSIZE - 12, .name_len = 2, .name = ".."};
        put_block(dev, bno, buf);
    }

    enter_name(pip, ino, bname); //gets to last entry in the block, allocates memory for new block, 
                                // puts entry inside existing block

    iput(mip); // release mip
    iput(pip); // release pip

    return ino;
}

void makedir()
{
    make_entry(1, pathname); // pass in 1 for directory and the pathname
}

void create_file() // page 336 
{
    make_entry(0, pathname); // pass in 0 for file and the pathname
}

int rmchild(MINODE * pip, char * name) // Adapted from algorithm in Professor Wang's book
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
                if(ideal != dp->rec_len) // Last entry since rec_len is just taking the rest of the block
                {
                    // Expand previous entry to take up deleted space.
                    prev->rec_len += dp->rec_len;
                }
                else if(prev == NULL && cp + dp->rec_len == buf + 1024)// First and only entry because prev 
                                                                //has not moved and dp takes the entire block
                {
                    /* deallocate the data block; reduce parent’s file size by BLKSIZE compact parent’s 
                    i_block[ ] array to eliminate the deleted entry if it’s between nonzero entries. */
                    char empty[BLKSIZE] = {0};
                    put_block(dev, pip->inode.i_block[i], empty);
                    bdalloc(dev, pip ->inode.i_block[i]); // Boof the entire block
                    pip->inode.i_size -= BLKSIZE; // Decrement the block by the entire size of a block
                    for(int j = i; j < 11; j++)
                    {
                        pip->inode.i_block[j] = pip->inode.i_block[j+1];
                    }
                    pip->inode.i_block[11] = 0;
                }
                else // Case where it is not (first and only) or last
                {
                    /* move all trailing entries LEFT to overlay the deleted entry add deleted 
                    rec_len to the LAST entry; do not change parent’s file size */
                    int removed = dp->rec_len;
                    char *temp = buf;
                    DIR *last = (DIR *)temp;

                    while(temp + last->rec_len < buf + BLKSIZE) // Move to last entry in dir
                    {
                        temp += last->rec_len;
                        last = (DIR *) temp;
                    }
                    last->rec_len = last->rec_len + removed;

                    // Move everything after the removed entry to the left by the length of the entry
                    memcpy(cp, cp + removed, BLKSIZE - current - removed + 1);
                }
                put_block(dev, pip->inode.i_block[i], buf);
                pip->dirty = 1;
                return 1;
            }
            cp += dp->rec_len; // move to next
            current += dp->rec_len;
            prev = dp;
            dp = (DIR *) cp;
        }
        return 0;
    }

    printf("ERROR: %s does not exist.\n",bname);
    return 1;
}

void rmdir() // Adapted from algorithm in Professor Wang's book
{
    dbname(pathname);
    char *temp = strdup(pathname);
    int ino = getino(temp); //2. Get inode number of pathname
    free(temp);
    if(!ino) // check if pathname exists 
    {
        printf("ERROR: %s does not exist.\n", pathname);
        return;
    }
    MINODE * mip = iget(dev, ino); //3. Get inode 
    if(running->uid != mip->inode.i_uid && running->uid != 0) // check if busy 
    {
        printf("ERROR: You do not have permission to remove %s.\n", pathname);
        iput(mip); // release mip
        return;
    }
    if(!S_ISDIR(mip->inode.i_mode)) // check if a directory
    {
        printf("ERROR: %s is not a directory", pathname);
        iput(mip); // release mip
        return;
    }
    if(mip->inode.i_links_count > 2) // check if directory is empty or not
    {
        printf("ERROR: Directory %s is not empty.\n", pathname);
        iput(mip); // release mip
        return;
    }
    if(mip->inode.i_links_count == 2) //empty but may contain regular files
    {
        char buf[BLKSIZE], * cp;
        DIR *dp;
        get_block(dev, mip->inode.i_block[0],buf);
        cp = buf;
        dp = (DIR *) buf;
        cp += dp->rec_len;
        dp = (DIR *) cp; // Move to second entry ".."
        if(dp->rec_len != 1012) // check if directory is empty 
        {
            printf("ERROR: Directory %s is not empty.\n", pathname);
            iput(mip); // release mip
            return;
        }
    }

    for(int i = 0; i < 12; i++) // for each data block of parent DIR do (assume: only 12 direct blocks)
    {
        if (mip->inode.i_block[i] == 0)
            continue;
        bdalloc(mip->dev,mip->inode.i_block[i]); // deallocate disk block
    }
    idalloc(mip->dev, mip->ino); // deallocates an inode (number). It clears the ino's bit in the 
                                 // device's inodes bitmap to 0. Then it increments the free inodes count 
                                 // in both super block and group descriptor by 1
    iput(mip); // release mip
    
    MINODE * pip = iget(dev, getino(dname)); // get new DIR node
    rmchild(pip, bname);

    pip->inode.i_links_count--; // decrement inode's link_count
    pip->inode.i_atime = pip->inode.i_mtime = time(0L);
    pip->dirty = 1; // mark as modified (dirty)
    iput(pip); // release pip
}


void link() // Creates a hard link from new_file to old_file. (Adapted from algorithm in Professor Wang's book)
{
    int ino = getino(pathname); // get inode number of pathname
    if(!ino) // check if file exists
    {
        printf("Source file does not exist\n");
        return;
    }
    MINODE * source = iget(dev, ino); //get inode
    if(S_ISDIR(source->inode.i_mode)) // check if directory (it cannot be one)
    {
        printf("Link to directory not allowed\n");
        return;
    }
    dbname(pathname2);
    int dirino = getino(dname);
    if(!dirino) // check if destination directory exists
    {
        printf("Destination directory does not exist\n");
        return;
    }
    MINODE * dirnode = iget(dev, dirino);
    if(!S_ISDIR(dirnode->inode.i_mode)) // check if destination is directory
    {
        printf("Destination is not a directory\n");
        return;
    }
    char *base = strdup(bname);
    if(search(dirnode, base)) // check if destination file already exists
    {
        printf("Destination file already exists\n");
        return;
    }

    free(base); // free memory
    enter_name(dirnode, ino, bname); // enter ino, bname as new dir_entry into a parent directory

    source->inode.i_links_count++; // increment inode's link_count by 1
    source->dirty = 1; // flag as modified (dirty)

    iput(source); // release source
    iput(dirnode); // release dirnode
}

void unlink() // Adapted from algorithm in Professor Wang's book
{
    int ino = getino(pathname); // get inode number of pathname
    if(!ino) // if path not found
    {
        printf("Could not find specified path\n");
        return;
    }
    MINODE * m = iget(dev, ino); // get inode
    if(S_ISDIR(m->inode.i_mode)) // check if it is a directory (it can't be one)
    {
        printf("Specified path is a directory\n");
        return;
    }
    if(--(m->inode.i_links_count) == 0) // if the links_count = 0 then remove filename
    {
        mytruncate(m); //at this time it only allocates the direct 1-12 blocks
        idalloc(m->dev, m->ino);
    }
    else
    {
        m->dirty = 1; // mark as modified (dirty)
    }

    /* remove name entry from parent DIR's data block */
    dbname(pathname);
    MINODE * parent = iget(dev, getino(dname)); // get new DIR node
    rmchild(parent, bname);

    iput(parent); // release parent
    iput(m); // release m
}

void symlink() // Adapted from algorithm in Professor Wang's book
{
    char * pathname_dup = strdup(pathname); // duplicate pathname string
    if(!getino(pathname_dup)) // check if source path exists
    {
        free(pathname_dup); // free memory of duplicated string
        printf("Specified source path does not exist\n");
        return;
    }
    free(pathname_dup); // free memory of duplicated string 
    int ino = make_entry(0, pathname2); // call to mkdir
    MINODE * m = iget(dev, ino); // get new DIR node
    m->inode.i_mode = 0xA1A4; // set as link type
    strcpy((char*)(m->inode.i_block), pathname);

    iput(m);
}

void readlink() // read contents of symbolic link file (Adapted from algorithm in Professor Wang's book)
{
    int ino = getino(pathname); // get the in-memory inode of the file
    if(!ino) // check if path exists
    {
        printf("Specified path does not exist\n");
        return;
    }
    MINODE * m = iget(dev, ino);
    if(!S_ISLNK(m->inode.i_mode)) // check if path is link file
    {
        printf("Specified path is not a link file\n");
        return;
    }
    printf("%s\n", (char*)(m->inode.i_block));

    iput(m);
}

/* Every file has an unique INODE data structure stats the file pointed to by filename and fills 
in the buf with stat information stat syscall finds the file's inode and copies information from inode 
into the stat structure except for dev and ino which are the file's device and inode numbers */
void mystat()
{
    int ino = getino(pathname); // get the in-memory inode of the file 
    if(!ino) // check to see if file exists or not 
    {
        printf("File not found\n");
        return;
    }
    MINODE * m = iget(dev, ino); // get new DIR node
    dbname(pathname); //gets the database information from pathname
    printf("File: %s\n", bname);
    printf("Size: %d\n", m->inode.i_size);
    printf("Blocks: %d\n", m->inode.i_blocks);

    char * type;
    /* check for type*/
    if(S_ISDIR(m->inode.i_mode)) // check if path is directory
        type = "Dir";
    else if(S_ISLNK(m->inode.i_mode)) // check if path is link file
        type = "Link";
    else // otherwise it is a regular file
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

void mychmod() /* changing the permissions of a file */
{
    int ino = getino(pathname); // get the in-memory inode of the file
    if(!ino) // check to see if file exists or not
    {
        printf("File does not exist\n");
        return;
    }
    /* get info from inode or modify the inode*/
    MINODE * m = iget(dev, ino); // get new DIR node
    int new;
    sscanf(pathname2, "%o", &new);
    
    int old = m->inode.i_mode;
    old >>= 9;  //change filename's permissions bits to octal value 
    new |= (old << 9);

    m->inode.i_mode = new;
    m->dirty = 1; // set dirty (modification flag) to 1 for write back

    iput(m);
}

void touch(char *name) // change filename timestamp (create file if it does not exist)
{
	int ino = getino(name);
    if(ino == -1) // if path does not exist
	{ 
		printf("not a path\n");
		return; // return because no path
	}
	MINODE *mip = iget(dev, ino); // get inode
	mip->inode.i_atime = mip->inode.i_mtime = time(0L); // modify time
	mip->dirty = 1;

	iput(mip);

	return;
} 


void quit() /* Adapted from the quit() function Professor Wang gave us*/
{
    /*iput() all minodes with (refCount > 0 && DIRTY)*/
    for (int i = 0; i < NMINODE; i++)
        if (minode[i].refCount > 0 && minode[i].dirty == 1)
        {
            minode[i].refCount = 1;
            iput(&(minode[i]));
        }
    exit(0); 
}




