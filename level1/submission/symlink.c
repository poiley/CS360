#include "globals.h"

#include <sys/stat.h>

void symlink() {
    char * pathname_dup = strdup(pathname); 
    
    if(!getino(pathname_dup)) {
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

void readlink() {
    int ino = getino(pathname); 
    
    if(!ino) {
        printf("Specified path does not exist\n");
        return;
    }
    
    MINODE * m = iget(dev, ino);
    
    if(!S_ISLNK(m->inode.i_mode)) {
        printf("Specified path is not a link file\n");
        return;
    }
    
    printf("%s\n", (char*) (m->inode.i_block));

    iput(m);
}