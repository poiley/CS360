#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>

#include "globals.h"


// given code
int get_block(int dev, int blk, char *buf)
{
    lseek(dev, blk*BLKSIZE, 0); //reposition read/write file offset
    return (read(dev, buf, BLKSIZE) < 0) ? 0 : 1; //returns a buffer containing valid data
}   

// given code
int put_block(int dev, int blk, char *buf)
{
    lseek(dev, blk*BLKSIZE, 0);
    return (write(dev, buf, BLKSIZE) != BLKSIZE) ? 0 : 1;
}   

int tokenize(char *pathname)
{
    // tokenize pathname into n components: name[0] to name[n-1];
    char * temp;
    int i = 0;
    temp = strtok(pathname, "/"); // stores string up till '/' in temp
    do
        name[i++] = temp;  // store each string in name array, incrementing by 1
    while((temp = strtok(NULL, "/")) && i < 64); // while theres more '/'
    return i;
}

MINODE *iget(int dev, int ino) {
    
    for(int i = 0; i < NMINODE; i++) //search in-memory nodes first
        if(minode[i].refCount > 0 && minode[i].dev == dev && minode[i].ino == ino) { // check if minode is found
            minode[i].refCount++;	// increment refcount
            return (minode + i); 	// return minode pointer to loaded INODE
        }

    // needed entry not in memory:
    int i;
    for(i = 0; i < NMINODE; i++) 
        if(minode[i].refCount == 0)
        {
            minode[i] = (MINODE){.refCount = 1, .dev = dev, .ino = ino};
            break;
        }

    char buf[BLKSIZE];
    int blk = (ino - 1) / 8 + inode_start, offset = (ino - 1) % 8; //write inode back to disk
    get_block(dev, blk, buf); // which inode is this block
    minode[i].inode = *((INODE*)buf + offset); //copy inode to minode.inode (next block)
    return minode + i;	// return minode pointer to loaded INODE
}


int iput(MINODE *mip) // dispose a used minode by mip
{
    if (--(mip->refCount) == 0 || mip->dirty) //still has user or no need to write back
    {
        char buf[BLKSIZE];
        int blk = (mip->ino - 1) / 8 + inode_start, offset = (mip->ino - 1) % 8; //write inode back to disk
        get_block(mip->dev, blk, buf); //get the block inode is on
        *((INODE*)buf + offset) = mip->inode; //copy INODE to inode in block 
        put_block(mip->dev, blk, buf); //write back to the disk
    }
}

int search(MINODE *mip, char *name)  // serach a directory INODE for entry with a given name
{
  char dbuf[BLKSIZE], temp[256];
  DIR *dp;
  char * cp;
  for (int i = 0; i < 12 && mip->inode.i_block[i]; i++) //search in the mnodes blocks
  {
    get_block(mip->dev, mip->inode.i_block[i], dbuf);
    dp = (DIR*) dbuf;
    cp = dbuf;			// cp equals buf

    while (cp < dbuf + BLKSIZE)		// traverse through cp while smaller than buf
    {
      strncpy(temp, dp->name, dp->name_len);	// store dp->name in temp variable
      temp[dp->name_len] = 0;		// makes end of temp string equal to 0
      
      if(strcmp(name,dp->name) == 0)	// if inode is found
      {
        return dp->inode;
      }
      cp += dp->rec_len; 	// else continue traversing through cp till found
      dp = (DIR*)cp;
    }
  }
  return 0; // return ino if found; return 0 if NOT
}
int getino(char *pathname)
{ 
    MINODE * cur = (pathname[0] == '/') ? root : running->cwd;  // if '/' then start at root, else start at cwd
    cur->refCount++;
   
    int size = tokenize(pathname);  // use tokenize to break up pathname into component strings - stored in global variables
    //if pathname is a single slash, name[0] will be null
    int ino;
    if(!name[0])		// if name[0] is null
        ino = cur->ino;
    else
        for(int i = 0; i < size; i++)
        {
            ino = search(cur, name[i]);  // search for the name in the directory
            if(ino == 0)
                return 0;
            iput(cur);
            cur = iget(cur->dev, ino);
        }
    iput(cur);
    return ino; // return inode number of pathname
}

int findmyname(MINODE *parent, u32 myino, char *myname) {	// for command pwd(running->cwd) which prints the absolute pathname of cwd
    char dbuf[BLKSIZE], temp[256];
    for(int i = 0; i < 12 && parent->inode.i_block[i]; i++)  // traverse through blocks
    {
        get_block(parent->dev, parent->inode.i_block[i], dbuf); // parent-> at a DIR minode
        char * cp = dbuf;
        DIR * dp = (DIR*)dbuf;
 
        while(cp < dbuf + BLKSIZE)	// while cp is smaller than buffer
        {
            if(dp->inode == myino)	// if ino in dir dp is found
            {
                strncpy(myname, dp->name, dp->name_len);   // copy entry name into myname[]
                return 1;
            }
            cp += dp->rec_len; // increments cp by rec_len
            dp = (DIR*)cp;
        }
    }
    return 0;
}


int tst_bit(char * buf, int bit)	// checks to see if bit is set
{
    return buf[bit/8] & (1 << (bit%8));
}

void set_bit(char * buf, int bit)	// set bit
{
    buf[bit/8] |= (1 << (bit%8));
}

void clr_bit(char * buf, int bit)	// clear bit
{
    buf[bit/8] &= ~(1 << (bit%8));
}

int decFreeInodes(int dev)
{
    char buf[BLKSIZE];
    
    get_block(dev, 1, buf);
    ((SUPER*)buf)->s_free_inodes_count--;   // decrement free inodes count in SUPER
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    ((GD*)buf)->bg_free_inodes_count--;   // decrement free inodes count in GD
    put_block(dev, 2, buf);
}

int incFreeInodes(int dev)
{
    char buf[BLKSIZE];
    
    get_block(dev, 1, buf);
    ((SUPER *)buf)->s_free_inodes_count++; // increment free inodes count in SUPER
    put_block(dev, 1, buf);			// update device

    get_block(dev, 2, buf);
    ((GD *)buf)->bg_free_inodes_count++;  // increment free inodes count in GD
    put_block(dev, 2, buf);		// update device
}

//Allocate and Inode 
int ialloc(int dev)
{
    char buf[BLKSIZE];
    get_block(dev, imap, buf);   // gets block from device and stores in buffer
    for(int i = 0; i < ninodes; i++)
        if(tst_bit(buf, i) == 0)  // if found
        {
            set_bit(buf, i);	// sets bit
            decFreeInodes(dev);	// decreases free inodes int by 1
            put_block(dev, imap, buf);
            return i + 1;
        }
    return 0;
}

//Allocate a disk block
int balloc(int dev)
{
    char buf[BLKSIZE];

    // read Group Descriptor 0 to get bmap, imap and iblock numbers
    get_block(dev, bmap, buf);
    for(int i = 0; i < nblocks; i++)	// for amount of nblocks
    {
        if(tst_bit(buf, i) == 0)	// see if bit is set yet
        {
            set_bit(buf, i);	// sets bit
            decFreeInodes(dev);
            put_block(dev, bmap, buf);	// creates new block to be committed
            return i + 1;		// returns FREE disk block number
        }
    }
        
    return 0;
}

//deallocate an Inode
void idalloc(int dev, int ino)
{
    char buf[BLKSIZE];
    get_block(dev, imap, buf);
    if(ino > ninodes)		// checks to see if ino is out of range
    {
        printf("ERROR: inumber %d out of range.\n", ino);
        return;
    }
    get_block(dev,imap,buf);	// gets imap block from device and stores in buffer
    clr_bit(buf,ino-1);		// clears bit
    put_block(dev,imap,buf);	// creates new block in device
    incFreeInodes(dev);		// increment free inodes by 1
}

//deallocate a disk block 
void bdalloc(int dev, int bno)
{
    char buf[BLKSIZE];
    if(bno > nblocks)  // checks to see if bno is out of range
    {
        printf("ERROR: bnumber %d out of range.\n", bno);
        return;
    }
    get_block(dev,bmap,buf); // read Group Descriptor to get bmap, imap and iblock numbers
    clr_bit(buf,bno-1);		// clears bit
    put_block(dev,bmap,buf);	// creates new block
    incFreeInodes(dev);		// subtracts from inode integer var
}

void mytruncate(MINODE * mip) // deallocates ALL the data blocks of INODE
{
    for(int i = 0; i < 12 && mip->inode.i_block[i]; i++)
        bdalloc(mip->dev, mip->inode.i_block[i]); // deallocate its INODE

    //todo: indirect blocks
}

void dbname(char *pathname)
{
    char temp[256];
    strcpy(temp, pathname);	// make temporary cpy of pathname
    strcpy(dname, dirname(temp));	// store directory name in dname
    strcpy(temp, pathname);
    strcpy(bname, basename(temp));	// store basename in bname var
}

void zero_block(int dev, int blk)
{
    char buf[BLKSIZE] = {0};
    put_block(dev, blk, buf);	// sets block = to 0s
}
