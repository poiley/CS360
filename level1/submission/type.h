#pragma once
#include <ext2fs/ext2_fs.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;   

#define BLKSIZE  1024
#define NMINODE    64
#define NOFT       64
#define NFD        10
#define NMOUNT      4
#define NPROC       2

/* in memory inodes structure */
typedef struct minode{
  INODE inode; // disk inode
  int dev, ino, refCount, dirty, mounted; // dirty is a modified flag, mounted is a mounted flag
  struct mntable *mptr;
}MINODE;

#define R 0
#define W 1
#define RW 2
#define APPEND 3

/* open file table structure */
typedef struct oft{ 
  int  mode, refCount, offset;
  MINODE *mptr; // mount table pointer
}OFT;

/* process control block structure which contains all the information of a process */
typedef struct proc{ 
  struct proc *next; // next PROC pointer
  int          pid, uid, gid;
  MINODE      *cwd;
  OFT         *fd[NFD];
}PROC;