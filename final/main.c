/*
 * File:  main.c 
 * By: Benjamin Poile and Lovee Baccus
 * Cpts 360 - Fall 2020
 */
#include "util.c"
#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "symlink.c"
#include "link_unlink.c"
#include "cp.c"
#include "mv.c"
#include "open_close_lseek.c"
#include "read_cat.c"
#include "write.c"


// GLOBAL VARIABLES
MINODE minode[NMINODE];
MINODE *root;

PROC proc[NPROC], *running;
OFT oft[NOFT];
MTABLE mtable[NMTABLE];

char default_disk[10] = "diskimage";
char gline[128];    // global for tokenized components
char *name[64];     // assume at most 32 components in pathname
int  nname;         // number of component strings
char *disk;

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start; // disk parameters

/*
 * Function: init 
 * Author: Ben Poile
 * --------------------
 *  Description: Initializes and populates required file system variables 
 *  Params: void
 *  Return: int
 *              0 for a successful run
 */
int init() {
    int i, j;
    MINODE *mip;
    PROC   *p;
    MTABLE *minodePtr; 

    for (i = 0; i < NMINODE; i++) {
        mip = &minode[i];
        mip->dev = mip->ino = 0;
        mip->refCount = 0;
        mip->mounted = 0;
        mip->mntptr = 0;
    }

    for (i = 0; i < NPROC; i++) {
        p = &proc[i];
        p->pid = i;
        p->uid = p->gid = 0;
        p->cwd = 0;
        p->status = FREE;
        for (j = 0; j < NFD; j++)
            p->fd[j] = 0;
    }

    for (i = 0; i < NMTABLE; i++) {
        minodePtr = &mtable[i];
        minodePtr->dev=0;
        minodePtr->mntDirPtr=0;
    }

    return 0;
}

/*
 * Function: mount_root 
 * Author: Ben Poile and Lovee Baccus
 * --------------------
 *  Description: Mounts the filesystem by calling iget on the global var 'dev' and assigns it to the MINODE struct root.
 *  Params: void
 *  Return: int
 *              0 for a successful run
 */
int mount_root() {
    root = iget(dev, 2);
    return 0;
}

/*
 * Function: main 
 * Author: Ben Poile and Lovee Baccus
 * --------------------
 *  Description: The purpose of the main function in this program is to handle initial error checking, program setup, and the shell.
 *  Params: void
 *  Return: int
 *              0 for a successful run
 *              1 for a failed run
 */
int main(int argc, char *argv[ ]) {
    char buf[BLKSIZE];
    char line[128], cmd[32], src[128], dest[128];

    if (argc > 1) // If there are user args
        disk = argv[1];
    else {
        printf("No disk parameter given. Using the default disk '%s' instead.\n", default_disk);
        disk = default_disk;
    }
 
    printf("[DEBUG] Opening filesystem.\n");
    if ((fd = open(disk, O_RDWR)) < 0) {
        printf("[ERROR] in main(): Open %s failed. Exiting.\n", disk);
        exit(1);
    }

    dev = fd; // fd is the global dev 

    // Read in the superblock
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;

    // Ensure filesystem is of type ext2
    if (sp->s_magic != 0xEF53){
        printf("[DEBUG] magic signature: %x\n[ERROR] in main(): Superblock states that this is not an ext2 filesystem. Exiting.\n", sp->s_magic);
        exit(1);
    }

    printf("[DEBUG] Filesystem is ext2 and ready for reading.\n");
    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;

    get_block(dev, 2, buf); 
    gp = (GD *)buf;

    bmap = gp->bg_block_bitmap;
    imap = gp->bg_inode_bitmap;
    inode_start = gp->bg_inode_table;
    printf("[DEBUG] bmp: %d\timap: %d\tinode_start: %d\n", bmap, imap, inode_start);

    init();  
    mount_root();
    printf("[DEBUG] root refCount = %d\n", root->refCount);

    printf("[DEBUG] Creating process 0 as running process.\n");
    running = &proc[0];
    proc[0].uid = 0;
    running->status = READY;
    running->cwd = iget(dev, 2);
    printf("[DEBUG] root refCount: %d\n", root->refCount);
    
    // Create P1 as a USER process
    proc[1].uid = 1;
    proc[1].status = FREE;
    
    while(1) {
        printf("Running Process %d - Available Commands: \n[ ls | cd | pwd | mkdir | rmdir  | creat | symlink | link | unlink ]\n[ open | read | lseek | pfd | write | close | cat | cp | mv | quit ] \n", running->uid);
        printf("user@FinalProject ~> ");
        
        fgets(line, 128, stdin);
        line[strlen(line)-1] = 0;

        *src = *dest = 0; // reset src and dest strings

        if (line[0] == 0) // If there's no input from the user
            continue;

        sscanf(line, "%s %s %s", cmd, src, dest);
        printf("[DEBUG] Command: %s\tSource: %s\tDestination: %s\n", cmd, src, dest);

        src[strlen(src)] = 0;
        dest[strlen(dest)] = 0;

        if (strcmp(cmd, "ls") == 0)
            ls(src);
        else if (strcmp(cmd, "cd") == 0)
            cd(src);
        else if (strcmp(cmd, "pwd") == 0)
            pwd(running->cwd);
        else if (strcmp(cmd, "mkdir") == 0)
            make_dir(src);
        else if (strcmp(cmd, "rmdir") == 0)
            rm_dir(src);
        else if (strcmp(cmd, "creat") == 0 || strcmp(cmd, "touch") == 0)
            creat_file(src);
        else if (strcmp(cmd, "symlink") == 0)
            sym_link(src, dest);
        else if (strcmp(cmd, "link") == 0)
            link_file(src, dest);
        else if (strcmp(cmd, "unlink") == 0)
            unlink_file(src);
        else if (strcmp(cmd, "open") == 0)
            open_file(src, dest);
        else if(strcmp(cmd, "read") == 0)
            read_file(src, dest);
        else if(strcmp(cmd, "cat") == 0)
            cat_file(src);
        else if(strcmp(cmd, "close") == 0)
            close_file(atoi(src));
        else if(strcmp(cmd, "lseek") == 0)
            lseek_file(atoi(src), atoi(dest));
        else if (strcmp(cmd, "cp") == 0)
            cp_file(src, dest);
        else if (strcmp(cmd, "mv") == 0)
            mv_file(src, dest);
        else if (strcmp(cmd, "write") == 0)
            write_file();
        else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0)
            quit();
    }
}

/*
 * Function: quit 
 * Author: Ben Poile
 * --------------------
 *  Description: Close filesystem and exit the program.
 *  Params: void
 *  Return: int
 *              0 for a successful exit
 */
int quit() {
    MINODE *mip;
    for (int i = 0; i < NMINODE; i++) {
        mip = &minode[i];
        if (mip->refCount > 0)
            iput(mip);
    }
    exit(0);
}
