#include "globals.h"

void cp_file() {
    int fd = myopen(pathname, 0);
    //printf("fd = %d\n", fd);
    if (fd >= 0) {
        int dev = running->cwd->dev;
        int ino = ialloc(dev);
        int blk = balloc(dev);
        if (mymk(0, parameter, ino, blk, 0) >= 0) {
            int gd = myopen(parameter, 1);
            if (gd >= 0) {
                int n;
                char buf[BLKSIZE];

                while (n = myread(fd, &buf, BLKSIZE, 2)) {
                    mywrite(gd, buf, n, 0);
                }

                close_file_helper(gd);
            } else {
                rm_helper(0);
                printf("Well, some problem occur in dst\n");
            }
        } else {
            printf("Failed to copy des file!\n");
        }
        close_file_helper(fd);
    } else {
        printf("Error occur because of the source file\n");
    }
}

int myopen(char *path, int mode) {
    MINODE *mip = iget(running->cwd->dev, getino(path));
    mip->refCount--;

    if (S_ISLNK(mip->INODE.i_mode)) {
        char buf[60];
        read_link((char *)mip->INODE.i_block, &buf);
        int gd = myopen(buf, mode);
        return gd;
    } else {
        if (mip != NULL) {
            int i = myopen_file(path, mode);
            return i;
        } else {
            printf("Open failed!\n");
            return -1;
        }
    }
}

int myopen_file(char *path, int mode) {
    MINODE *mip = iget(running->cwd->dev, getino(pathname));
    OFT *oftp = 0;
    int fd = chkFd(mip);

    if (fd == -2) {
        mip->refCount--;
        return -2;
    }

    if (fd == -1) {
        fd = falloc();
    }

    if (fd > -1) {
        if (mode == 0 | mode == 1 | mode == 2 | mode == 3) {
            oftp = malloc(sizeof(OFT));

            switch (mode) {
                case 0:
                    oftp->offset = 0;
                    mip->INODE.i_atime = do_touch();
                    break;
                case 1:
                    truncates(mip);
                    oftp->offset = 0;
                    mip->INODE.i_atime = mip->INODE.i_mtime = do_touch();
                    break;
                case 2:
                    oftp->offset = 0;
                    mip->INODE.i_atime = mip->INODE.i_mtime = do_touch();
                    break;
                case 3:
                    oftp->offset = mip->INODE.i_size;
                    mip->INODE.i_atime = mip->INODE.i_mtime = do_touch();
                    break;
            }

            oftp->mode = mode;
            oftp->refCount = 1;
            oftp->inodeptr = mip; // point at the file's minode[]

            running->fd[fd] = oftp;

            oftp->inodeptr->dirty = 1;
            oftp->inodeptr->refCount++;

            iput(oftp->inodeptr);

            return fd;
        } else {
            printf("invalid mode\n");
            mip->refCount--;
            return (-1);
        }
    } else {
        mip->refCount--;
        printf("Already running/opening too many, or file had been editing\n");
        return -1;
    }
}

int write_file() {
    int fd = atoi(pathname);
    OFT *ofpt = running->fd[fd];

    if (S_ISLNK(ofpt->inodeptr->INODE.i_mode)) {
        char par[128], buf[60];

        strcpy(par, parameter);

        read_link((char *)ofpt->inodeptr->INODE.i_block, &buf);

        strcpy(pathname, buf);
        if (ofpt->mode == 1)
            strcpy(parameter, "W");
        else if (ofpt->mode == 3)
            strcpy(parameter, "APPEND");

        int gd = open_file_helper();

        char *g = gd + '0';

        pathname[0] = g;
        pathname[1] = 0;
        strcpy(parameter, par);

        int z = write_file();

        close_file_helper(gd);
        printf("\n");

        return z;
    } else {
        if (ofpt != 0 && (ofpt->mode == 1 | ofpt->mode == 3)) {
            char buff[BLKSIZE];

            memset(buff, 0, BLKSIZE);
            printf("input the string :");
            scanf("%s", &buff);
            //printf("Inputed %s\n", buff);
            int nbytes = atoi(parameter);
            //printf("nbytes = %d\n", nbytes);
            int i = mywrite(fd, buff, nbytes, 1);

            //printf("%s\n", buff);
            //printf("After write: fd->ino->refCount = %d\n", ofpt->refCount);
            return i;
        } else {
            printf("File is not opened for W or APPEND, or bad input. Open failed!\n");
            return -1;
        }
    }
}

int mywrite(int fd, char *buf, int nbytes, int prt) {
    int count = 0;
    int lbk, startByte, blk, remain;
    OFT *oftp = running->fd[fd];
    char wbuf[BLKSIZE];
    memset(wbuf, 0, BLKSIZE);
    char *cq = buf;

    while (nbytes > 0) {
        lbk = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;

        if (lbk < 12) {
            if (oftp->inodeptr->INODE.i_block[lbk] == 0) 
                oftp->inodeptr->INODE.i_block[lbk] = balloc(oftp->inodeptr->dev);

            blk = oftp->inodeptr->INODE.i_block[lbk];
        } else if (lbk >= 12 && lbk < 256 + 12) {
            if (oftp->inodeptr->INODE.i_block[12] == 0)
                oftp->inodeptr->INODE.i_block[12] = balloc(oftp->inodeptr->dev);

            get_block(running->cwd->dev, oftp->inodeptr->INODE.i_block[12], &wbuf);
            blk = *((long *)wbuf + (lbk - 12));
        } else {
            printf("double indirect blocks\n");
            if (oftp->inodeptr->INODE.i_block[13] == 0)
                oftp->inodeptr->INODE.i_block[13] = balloc(oftp->inodeptr->dev);

            get_block(running->cwd->dev, oftp->inodeptr->INODE.i_block[13], &wbuf);
            blk = *((long *)wbuf + ((lbk - 256 - 12) / 256));
            if (blk == 0) {
                blk = balloc(oftp->inodeptr->dev);
                put_block(running->cwd->dev, oftp->inodeptr->INODE.i_block[13], (char *)blk);
            }

            get_block(running->cwd->dev, blk, &wbuf);
            blk = *((long *)wbuf + ((lbk - 256 - 12) % 256));
        }
        
        get_block(oftp->inodeptr->dev, blk, &wbuf);
        char *cp = wbuf + startByte;
        remain = BLKSIZE - startByte;

        while (remain > 0) { // write as much as remain allows

            *cp++ = *cq++; // cq points at buf[ ]
            count++;
            nbytes--;
            remain--;                                        // dec counts
            oftp->offset++;                                  // advance offset
            if (oftp->offset > oftp->inodeptr->INODE.i_size) // especially for RW|APPEND mode
                oftp->inodeptr->INODE.i_size++;              // inc file size (if offset > fileSize)
            if (nbytes <= 0)
                break; // if already nbytes, break
        }
        put_block(oftp->inodeptr->dev, blk, &wbuf); // write wbuf[ ] to disk

        // loop back to while to write more .... until nbytes are written
    }

    oftp->inodeptr->dirty = 1; // mark mip dirty for iput()
    //iput(oftp->inodeptr);
    if (prt == 1)
    {
        printf("wrote %d char into file descriptor fd=%d\n", count, fd);
    }
    //return nbytes;
    return count;
}

int read_file()
{

    int fd = atoi(pathname);
    int nbytes = atoi(parameter);

    OFT *ofpt = running->fd[fd];

    if (S_ISLNK(ofpt->inodeptr->INODE.i_mode))
    {

        //char buf[60], buff[nbytes];
        //char *f = fd+'0';
        //char b[23]="/lost+found/reading/fdx";
        //b[22]=f;
        //char c[3]="fdx";

        char par[128], buf[60];

        strcpy(par, parameter);

        //read_link(b, &buf);//buf = path to symlink itself
        //printf("path to symlink itself = %s\npath in symlink's block = %s\n", buf, buff);

        read_link((char *)ofpt->inodeptr->INODE.i_block, buf); //buff = path in symlink's blok

        //printf("the linked path is '%s'\n", buff);

        strcpy(pathname, buf);
        if (ofpt->mode == 0)
        {
            strcpy(parameter, "R");
        }
        else if (ofpt->mode == 2)
        {
            strcpy(parameter, "RW");
        }
        //printf("pathname = %s\n", pathname);

        int gd = open_file_helper();

        //printf("gd = %d\n", gd);
        //myread(gd, &buff, nbytes, 1);

        char *g = gd + '0';
        //c[2]=g;

        //printf("gd = %c\n", g);
        //printf("Par = %s\n", par);

        pathname[0] = g;
        pathname[1] = 0;
        strcpy(parameter, par);

        int z = read_file();

        close_file_helper(gd);
        printf("\n");

        return z;
    }
    else
    {
        if (ofpt != 0 && (ofpt->mode == 0 | ofpt->mode == 2) && nbytes >= 0)
        {
            char buf[nbytes];

            int i = myread(fd, &buf, nbytes, 1);

            //printf("%s\n", buf);

            return i;
        }
        else
        {
            printf("File is not opened for R or RW, or bad input. Open failed!\n");

            return -1;
        }
    }
}

int myread(int fd, char *buf, int nbytes, int prt)
{
    OFT *oftp = running->fd[fd];

    int count = 0;

    int avil = oftp->inodeptr->INODE.i_size - oftp->offset;

    //printf("avaible is %d\tsize is %d\toffset is %d\n", avil,oftp->inodeptr->INODE.i_size, oftp->offset);
    int lbk, startByte, blk, remain;

    char readbuf[BLKSIZE];
    char *cq = buf; // cq points at buf[ ]

    //printf("\n*****************************************************************\n");

    while (nbytes && avil)
    {

        //Compute LOGICAL BLOCK number lbk and startByte in that block from offset;

        lbk = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;

        // I only show how to read DIRECT BLOCKS. YOU do INDIRECT and D_INDIRECT

        //printf("block number is %d\n", lbk);

        if (lbk < 12)
        {                                             // lbk is a direct block
            blk = oftp->inodeptr->INODE.i_block[lbk]; // map LOGICAL lbk to PHYSICAL blk
        }
        else if (lbk >= 12 && lbk < 256 + 12)
        {
            //  indirect blocks

            get_block(running->cwd->dev, oftp->inodeptr->INODE.i_block[12], &readbuf);

            //blk = ((long)readbuf / BLKSIZE) + (lbk-12);
            blk = *((long *)readbuf + (lbk - 12));
        }
        //else if (lbk >= 12+256 && lbk < 256*256 + 12) {
        else
        {
            //  double indirect blocks
            get_block(running->cwd->dev, oftp->inodeptr->INODE.i_block[13], &readbuf);

            blk = *((long *)readbuf + ((lbk - 12) / 256));

            get_block(running->cwd->dev, blk, &readbuf);

            //blk = ((long)readbuf / BLKSIZE) + ((lbk - 12)%256);
            blk = *((long *)readbuf + ((lbk - 256 - 12) % 256));
        }
        /*
       else
       {
           //triple link, just for fun
           get_block(running->cwd->dev, oftp->inodeptr->INODE.i_block[14], &readbuf);

           blk = (reafbuf / BLKSIZE)  + (((lbk - 12)/256)/256);

           get_block(running->cwd->dev, blk, &readbuf);

           blk = (readbuf / BLKSIZE) + (((lbk - 12)/256)%256);

           get_block(running->cwd->dev, blk, &readbuf);
           blk = (readbuf / BLKSIZE) + ((lbk - 12)%(256*256));
       }
*/
        //get the data block into readbuf[BLKSIZE]
        get_block(running->cwd->dev, blk, &readbuf);

        // copy from startByte to buf[ ], at most remain bytes in this block
        char *cp = readbuf + startByte;
        remain = BLKSIZE - startByte; // number of bytes remain in readbuf[]
        while (remain > 0)
        {

            if (prt == 1)
            {
                printf("%c", *cp);
            }
            *cq++ = *cp++;  // copy byte from readbuf[] into buf[]
            oftp->offset++; // advance offset
            count++;        // inc count as number of bytes read
            avil--;
            nbytes--;
            remain--;
            if (nbytes <= 0 || avil <= 0)
                break;
        }

        // if one data block is not enough, loop back to OUTER while for more ...
    }

    if (prt < 1)
    {
        printf("\nmyread: read %d char from file descriptor %d\n", count, fd);
    }

    //printf("\ncount = %d\n", count);

    if (count == 0 & prt < 2)
    {
        printf("\n*****************************************************************\n");
    }

    return count; // count is the actual number of bytes read
}

open_file()
{

    int fd = open_file_helper();

    if (fd > -1)
    {

        char *d = "/lost+found/reading";

        char *f = fd + '0';

        char b[23] = "/lost+found/reading/fdx";
        b[22] = f;

        if (search(running->cwd->dev, d, 1) == -1)
        {
            mymk(1, d, ialloc(running->cwd->dev), balloc(running->cwd->dev), 0);
        }

        if (search(running->cwd->dev, b, -1) == -1)
        {
            symlinks(pathname, b);
        }
    }

    return fd;
}

//ofpt points to "real" file
//check is the "real" block had been cleaned when deleted the origin

int open_file_helper()
{
    char cmode[6];

    MINODE *mip = iget(running->cwd->dev, getino(running->cwd->dev, pathname, 0));

    //printf("from open_file_helper: pathname=%s\n", pathname);

    OFT *oftp = 0;
    int mode;

    int fd = chkFd(mip);

    //if opened for editing
    if (fd == -2)
    {
        mip->refCount--;
        return -2;
    }

    //if not opened (for R) yet
    if (fd == -1)
    {
        fd = falloc();
    }

    memset(cmode, 0, sizeof(cmode));

    strncpy(cmode, parameter, sizeof(cmode));

    if (fd > -1)
    {
        mode = transMode(cmode);
        //printf("mode = %d\n", mode);
        if (mode == 0 | mode == 1 | mode == 2 | mode == 3)
        {
            oftp = malloc(sizeof(OFT));

            switch (mode)
            {
            case 0:
                oftp->offset = 0; // R: offset = 0
                mip->INODE.i_atime = do_touch();
                break;
            case 1:
                truncates(mip); // W: truncate file to 0 size
                oftp->offset = 0;
                mip->INODE.i_atime = mip->INODE.i_mtime = do_touch();
                //printf("1.refcount = %d\n", mip->refCount);
                break;
            case 2:
                oftp->offset = 0; // RW: do NOT truncate file
                mip->INODE.i_atime = mip->INODE.i_mtime = do_touch();
                break;
            case 3:
                oftp->offset = mip->INODE.i_size; // APPEND mode
                mip->INODE.i_atime = mip->INODE.i_mtime = do_touch();
                break;
            }

            oftp->mode = mode; // mode = 0|1|2|3 for R|W|RW|APPEND
            oftp->refCount = 1;
            oftp->inodeptr = mip; // point at the file's minode[]

            running->fd[fd] = oftp;

            oftp->inodeptr->dirty = 1;
            oftp->inodeptr->refCount++;

            //printf("from open: size = %d\n", oftp->inodeptr->INODE.i_size);
            //printf("1.refcount = %d\n", mip->refCount);
            iput(oftp->inodeptr);

            return fd;
        }
        else
        {
            printf("invalid mode\n");
            mip->refCount--;
            return (-1);
        }
    }
    else
    {
        mip->refCount--;
        printf("Already running/opening too many, or file had been editing\n");
        return -1;
    }
}

read_link(char *path, char *buf)
{
    MINODE *mip = iget(running->cwd->dev, getino(running->cwd->dev, path, 0));

    mip->refCount--;

    if (S_ISLNK(mip->INODE.i_mode))
    {
        //Correct? Need to Check
        char pwd[60];
        strcpy(pwd, (char *)mip->INODE.i_block);
        read_link(pwd, buf);
        return;
    }

    strcpy(buf, path);
}

close_file()
{
    int fd = atoi(pathname);
    int gd = close_file_helper(fd);

    if (gd != -1)
    {
        char *d = "/lost+found/reading";
        char *f = fd + '0';
        char b[3] = "fdx";
        b[2] = f;

        MINODE *pip = iget(running->cwd->dev, getino(running->cwd->dev, d, 1));
        //pip->refCount++;
        rm_child(pip, b, 0);
        //printf("pip->refcount = %d, pip->ino=%d\n", pip->refCount, pip->ino);
    }

    //mymk(1, d, ialloc(running->cwd->dev));

    //symlinks(pathname, b);
}

int close_file_helper(int fd)
{
    OFT *oftp;

    if ((fd >= 0 && fd < 10) && running->fd[fd] != 0)
    {

        oftp = running->fd[fd];
        //printf("Before close: fd->ino->refCount = %d\n", oftp->inodeptr->refCount);
        running->fd[fd] = 0;
        oftp->refCount--;
        if (oftp->refCount > 0)
            return 0;

        // last user of this OFT entry ==> dispose of the Minode[]
        iput(oftp->inodeptr);

        //printf("After close: fd->ino->refCount = %d\n", oftp->inodeptr->refCount);
        return 0;
    }
    else
    {
        printf("Failed closing file\n");
        return -1;
    }
}
lseek_file()
{
    int fd = atoi(pathname);
    int gd = atoi(parameter);
    lseek_file_helper(fd, gd);
}

int lseek_file_helper(int fd, int position)
{

    OFT *oftp;

    if ((fd >= 0 && fd < 10) && running->fd[fd] != 0)
    {

        oftp = running->fd[fd];

        if (position >= 0 && position < running->fd[fd]->inodeptr->INODE.i_size)
        {
            int originalPosition = oftp->offset;

            oftp->offset = position;

            return originalPosition;
        }

        return -1;
    }
    else
    {
        printf("Failed closing file\n");
        return -1;
    }
}