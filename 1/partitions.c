#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define sector_size 512

char buf[sector_size];
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
int vdisk;

typedef struct partition {
	u8 drive;             /* drive number FD=0, HD=0x80, etc. */

	u8  head;             /* starting head */
	u8  sector;           /* starting sector */
	u8  cylinder;         /* starting cylinder */

	u8  sys_type;         /* partition type: NTFS, LINUX, etc. */

	u8  end_head;         /* end head */
	u8  end_sector;       /* end sector */
	u8  end_cylinder;     /* end cylinder */

	u32 start_sector;     /* starting sector counting from 0 */
	u32 nr_sectors;       /* number of of sectors in partition */
} Partition;

int main(void) {
    char start_sector[sector_size];
    vdisk = open("vdisk", O_RDONLY);

    read(vdisk, buf, sector_size);
    memcpy(start_sector, buf, sector_size);

    Partition *MBR = (Partition *) &start_sector[0x1BE];
     
	for(int i = 0; i < 4; i++, MBR++) {
        partInfo(MBR);
        if(MBR->sys_type == 5)
            partEXT(MBR, MBR->start_sector);
	}
}


void partInfo(Partition *p) { printf("sys_type = '%d\nstart_sector = %u\nend_sector %u\nnr_sectors = %u\n\n", p->sys_type, p->start_sector, p->start_sector + p->nr_sectors - 1, p->nr_sectors); }

void partEXT(Partition *p, u32 sector) {
	u32 n_sector;

	if(p->start_sector == 0) 
		return;

    n_sector = p->start_sector;
    if (sector != p->start_sector)
        n_sector += sector;

    lseek(vdisk, (long) n_sector * sector_size, SEEK_SET);
    read(vdisk, buf, sector_size);

    p = (Partition *) &buf[0x1BE];
    printf("sys_type = '%d\nstart_sector = %u\nend_sector %u\nnr_sectors = %u\n\n", p->sys_type, n_sector + p->start_sector, n_sector + p->start_sector + p->nr_sectors - 1, p->nr_sectors);

    partEXT(p++, sector);
}
