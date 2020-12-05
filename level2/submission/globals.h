#pragma once

#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256], pathname2[256], dname[256], bname[256];
extern int data_start;
extern OFT oft[NOFT];

void mychdir();
void ls_file(int inom, char *filename);
void ls_dir(char *dirname);
void ls();
void rpwd(MINODE *wd);
void pwd();
int ideal_length(int name_len);
void enter_name(MINODE *pip, int myino, char *myname);
int make_entry(int dir, char *name);
void makedir();
void create_file();
int rmchild(MINODE *pip, char *name);
void rmdir();
void link();
void unlink();
void symlink();
void readlink();

int get_block(int dev, int blk, char *buf);
int put_block(int dev, int blk, char *buf);
int tokenize(char *pathname);
MINODE *iget(int dev, int ino);
int iput(MINODE *mip);
int search(MINODE *mip, char *name);
int getino(char *pathname);
int findmyname(MINODE *parent, u32 myino, char *myname);
int tst_bit(char *buf, int bit);
void set_bit(char *buf, int bit);
void clr_bit(char *but, int bit);
int decFreeInodes(int dev);
int incFreeInodes(int dev);
int ialloc(int dev);
int balloc(int dev);
void idalloc(int dev, int ino);
void bdalloc(int dev, int bno);
void mytruncate(MINODE *mip);
void dbname(char *pathname);
void zero_block(int dev, int blk);
void mystat();
void mychmod();
void touch(char *name);
void quit();
