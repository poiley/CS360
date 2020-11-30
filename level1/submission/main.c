#include "globals.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

MINODE minode[NMINODE];
MINODE *root;
PROC proc[NPROC], *running;
OFT oft[NOFT];

char gpath[256];
char *name[64]; 
int  n;

int  fd, dev;
int  nblocks, ninodes, bmap, imap, inode_start;
char line[256], cmd[32], pathname[256], pathname2[256], dname[256], bname[256];
int data_start;

void init(void) {		
    proc[0].uid = 0;    
    proc[1].uid = 1;	
    running = proc;
    for(int i = 0; i < NMINODE; i++)
        minode[i].refCount = 0;
}

void mount_root(char * name) {
    dev = open(name, O_RDWR);  
    char buf[1024];

    get_block(dev, 1, buf);	
    SUPER* s = (SUPER*) buf; 	

    if(s->s_magic != 0xEF53) {
        printf("NOT AN EXT2 FILESYSTEM!\n");
        exit(0); 
    }

    data_start = s->s_first_data_block;
    nblocks = s->s_blocks_count;    
    ninodes = s->s_inodes_count;    

    get_block(dev, 2, buf);		
    GD * g = (GD*)buf; 

    bmap = g->bg_block_bitmap;		
    imap = g->bg_inode_bitmap;		
    inode_start = g->bg_inode_table;	

    root = iget(dev, 2);	   	

    proc[0].cwd = iget(dev, 2);		
    proc[1].cwd = iget(dev, 2);		
}


int main(int argc, char * argv[]) {
    if (argc < 2) {
        printf("Usage: fs360 diskname\n");
        exit(1); 
    }

    init();
    mount_root(argv[1]); 

    void (*fptr[]) () = { (void (*)()) makedir, rmdir, mychdir, ls, pwd, create_file, mystat, link, symlink, unlink, mychmod, touch, quit };
    
    char *cmdNames[21] = { "mkdir", "rmdir", "cd", "ls", "pwd", "creat", "stat", "link", "symlink", "unlink", "chmod", "touch", "quit" }; 

    while(1) {
        cmd[0] = pathname[0] = pathname2[0] = '\0';
        int i;
	    printf("cmd $"); 
        fgets(line,256,stdin); 
        sscanf(line, "%s %s %[^\n]s", cmd, pathname, pathname2);  

        for(i = 0; i < 13 && (strcmp(cmd, cmdNames[i])); i++)
            if(i < 13)
                fptr[i]();
            else 
                printf("%s is not a valid function.\n", cmd);
    }
}
