#include <stdio.h>
#include <fcntl.h>

#include <string.h>
#include <stdlib.h>

#include "level1.h"
#include "type.h"
#include "util.h"

MINODE minode[NMINODE];
MINODE *root;
PROC proc[NPROC], *running;
OFT oft[NOFT];

char gpath[256];
char *name[64]; // assume at most 64 components in pathnames
int  n;

int  fd, dev;
int  nblocks, ninodes, bmap, imap, inode_start;
char line[256], cmd[32], pathname[256], pathname2[256], dname[256], bname[256];
int data_start;

void init(void) // Initialize data structures
{			
    proc[0].uid = 0;    // P0 with uid=0
    proc[1].uid = 1;	// P1 with uid=1
    running = proc;
    for(int i = 0; i < NMINODE; i++)
        minode[i].refCount = 0;
}

void mount_root(char * name)  // mount root file system, establish / and CWDs
{
    dev = open(name, O_RDWR);  // open device for RW (use the file descriptor as dev for the opened device)
    char buf[1024];

    get_block(dev, 1, buf);	// get block from device and store in buffer
    SUPER* s = (SUPER*)buf; 	// store super block from buffer in var s

    if(s->s_magic != 0xEF53)  // verify if super block is an EXT2 FS
    {
        printf("NOT AN EXT2 FILESYSTEM!\n");
        exit(0); // if not EXT2, exit
    }

    data_start = s->s_first_data_block;
    nblocks = s->s_blocks_count;    // record nblocks as globals
    ninodes = s->s_inodes_count;    // record ninodes as globals

    /* Read in group descriptor 0 (in block #2) to get block number of bmap, imap, inodes_start */
    get_block(dev, 2, buf);		// read group descriptor and store in buf
    GD * g = (GD*)buf; // stores buffer in GD (group descriptor)

    bmap = g->bg_block_bitmap;		// get bmap number from gb block
    imap = g->bg_inode_bitmap;		// get imap number from gb block
    inode_start = g->bg_inode_table;	// get inode number from gb block

    root = iget(dev, 2);	   	// get root node

    /* let cwd of both P0 and P1 point at the root minode*/
    proc[0].cwd = iget(dev, 2);		// Let cwd of P0 pt at root minode 
    proc[1].cwd = iget(dev, 2);		// Let cwd of P1 pt at root minode 
}


int main(int argc, char * argv[])
{
    if (argc < 2) // if we aren't passed in at least 2 arguments
    {
        printf("Usage: fs360 diskname\n");
        exit(1); // exit failure
    }
    init();
    mount_root(argv[1]); // mount what the user gives as the second argument as the root

    // table of function pointers
    void (*fptr[]) () = { (void (*)())makedir, rmdir,mychdir,ls,pwd,create_file,mystat,link,symlink,unlink, mychmod, touch, quit};
    // array of possible commands user could input, these correspond to above table of function pointers
    char *cmdNames[21] = {"mkdir", "rmdir", "cd", "ls", "pwd", "creat", "stat", "link", "symlink", "unlink", "chmod", "touch", "quit"}; 

    while(1)
    {
        cmd[0] = pathname[0] = pathname2[0] = '\0';
        int i;
	    printf("cmd $"); // ask user to input command
        fgets(line,256,stdin); // read user input
        sscanf(line, "%s %s %[^\n]s", cmd, pathname, pathname2);  // reads formatted user input and 
                                                                // separates user input into different variables - [^\n] 
                                                                // allows us to get everything up until the newline is reached

        for(i = 0;i < 13 && (strcmp(cmd,cmdNames[i])); i++); // for loop goes through each possible command to see if the user's
                                                            //  input matches one of our commands
        {
            if(i < 13)
            {
                fptr[i](); // calls to appropriate function
            }
            else // no matching command was found
            {
                printf("%s is not a valid function.\n", cmd);
            }
        }
    }
}
