/******** cd_ls_pwd.c ***********/

#include "cd_ls_pwd.h"

int ls_file(int ino) {
    MINODE *mip = iget(dev, ino);
	INODE stat = mip->INODE;
    // Use INODE contents to ls it 
	char ftime[64], mods[3] = { 'x', 'w', 'r' };
	int i;

	// print mode
	if (S_ISREG(stat.i_mode))
		putchar('-');
	else if (S_ISDIR(stat.i_mode))
		putchar('d');
	else if (S_ISLNK(stat.i_mode))
		putchar('l');

	for (i=8; i >= 0; i--)	
	{
		if (stat.i_mode & (1 << i))
			putchar(mods[i % 3]);
		else
			putchar('-');
	}
printf("%4d ", stat.i_links_count);
	printf("%4d ", stat.i_uid);
	printf("%4d ", stat.i_gid);
	printf("%8d ", stat.i_size);

	return 0;
}

int ls_dir(char *dirname) {
	char buf[BLKSIZE];
    int ino;
    MINODE *mip;
	int i;
	char temp[256], *cp;

	if(!strcmp(dirname, ""))
		mip = running->cwd;
	else {
		ino = getino1(dirname);
		mip = iget(dev, ino);
	}

	for (i=0; i < 12; i++) { 
		if (mip->INODE.i_block[i] == 0)
			break;
		get_block(dev, mip->INODE.i_block[i], buf);
	    dp = (DIR *)buf;
		cp = buf;

	    while (cp < buf + BLKSIZE) {
		   strncpy(temp, dp->name, dp->name_len);
	       temp[dp->name_len] = 0;

		   ls_file(dp->inode);
		   printf("%s\n", temp);
	       cp += dp->rec_len;
		   dp = (DIR *)cp;
		   
	    }
	}
	return 0;
}

int ls(char *name) {
    int ino;
    MINODE *mip;
	INODE check;

	if(!strcmp(name, "")) {
		mip = running->cwd;
		check = mip->INODE;
	} else {
		ino = getino1(name);
		mip = iget(dev, ino);
		check = mip->INODE;
	}
	
	// Check if reg or dir, use correct ls
	if (S_ISDIR(check.i_mode))
		ls_dir(name);
	else {
		ls_file(ino);
		printf("%s\n", basename(name));
	}

	return 1;
}

int chdir(char *pathname) {
	if(!strcmp(pathname, "")) {
		running->cwd = root;
		return 1;
	}

	int ino = getino1(pathname);
	MINODE *mip = iget(dev, ino);

	if(S_ISDIR(mip->INODE.i_mode)) {
		iput(running->cwd);
		running->cwd = mip;
		return 1;
	} else
		printf("%s is not a directory!\n", pathname);
	return 0;
}

int rpwd(MINODE *wd) {
	if(wd == root) {
		printf("/");
		return 0;
	}

	int pino = search(wd, "..");
	MINODE *pip = iget(dev, pino);
	char myname[64];

	findmyname(pip, wd->ino, myname);

	rpwd(pip);

	printf("%s/", myname);
	return 0;
}

int pwd(MINODE *wd) {
	if (wd == root)
		printf("/");
    else
         rpwd(wd);
	putchar('\n');
	return 1;
}
