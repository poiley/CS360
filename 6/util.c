/****** util.c *****/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"
#include "util.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];

int tokenize1(char *pathname) {
	int i=0;

	if(pathname[0] == '/')
		name[i++] = "/";

	name[i++] = strtok(pathname, "/");
	while(name[i++] = strtok(0, "/"));
	return i - 1;
}

int getino1(char *pathname) { 
	char temp[256];
	MINODE *mip;
	int n, i, ino;

	strcpy(temp, pathname);
	n = tokenize1(temp);

	if(!strcmp(name[0], "/")) {
		i = 1;
		mip = root;
	} else {
		i = 0;
		mip = running->cwd;
	}

	while(i < n) {
		if(!S_ISDIR(mip->INODE.i_mode)) {
			printf("%s is not a directory!", name[i - 1]);
			return 0;
		}
		ino = search(mip, name[i]);
		if(!ino)
		{
			printf("%s not found\n", name[i]);
			return 0;
		}
		
		mip = iget(dev, ino);
		i++;
	}
	
	return ino;
}



// THESE two functions are for pwd(running->cwd), which prints the absolute
// pathname of CWD. 
/*
int findmyname(MINODE *parent, u32 myino, char *myname) 
{
   // parent -> at a DIR minode, find myname by myino
   // get name string of myino: SAME as search except by myino;
   // copy entry name (string) into myname[ ];
}


int findino(MINODE *mip, u32 *myino) 
{
  // fill myino with ino of . 
  // retrun ino of ..
}
*/
