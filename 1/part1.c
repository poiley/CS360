#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <ext2fs/ext2_fs.h>  /* Ubuntu User: sudo apt install e2fslibs-dev */

#define sector_size 512

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

struct partition {	 /* partition struct */
	u8 drive;		 /* 0x80 - active */
	u8 head;		 /* starting head */
	u8 sector;		 /* starting sector */
	u8 cylinder;	 /* starting cylinder */
	u8 sys_type;	 /* partition type */
	u8 end_head;	 /* end head */
	u8 end_sector;	 /* end sector */
	u8 end_cylinder; /* end cylinder */
	u32 start_sector;/* starting sector counting from 0 */
	u32 nr_sectors;  /* nr of sectors in partition */
};

char buf[sector_size];
char *dev = "vdisk";
int fd;

int main() {
	struct partition *p;
	int vdiskNum = 1, extStart = 0, offset;

	fd = open(dev, O_RDONLY);   // open dev for READ

	read(fd, buf, sector_size); // read MBR into buf[]

	printf("Device\t\tStart\t\tEnd\t\tSectors\t\tSize\t\tID\n"); // fdisk table header

	p = (struct partition *) &buf[0x1be]; // p->P1

	while (p->sys_type) {
		printf("vdisk%d\t%8d\t%8d\t%8d\t\t%8dk %8x\n",
			   vdiskNum++,
			   p->start_sector,
			   p->start_sector + p->nr_sectors - 1,
			   p->nr_sectors,
			   p->nr_sectors * sector_size / 1024,
			   p->sys_type);

		if (p->sys_type == 5) {
			offset = 0;
			extStart += p->start_sector; // set extStart to the beginning location of the EXTENDED partiton

			struct partition *extPart = p; // creates a child partition object to loop through the EXTENDED partition variable p

			lseek(fd, extStart * sector_size, SEEK_SET); // go to extPart location in vdisk

			while (extPart->sys_type) { // while there are partitions to review within the EXTENDED partition
				read(fd, buf, sector_size); // read partition into buffer
				extPart = (struct partition *) &buf[0x1BE]; // assign partition data to extPart

				printf("vdisk%d\t%8d\t%8d\t%8d\t\t%8dk %8x\n",
					   vdiskNum++,
					   extPart->start_sector + offset + extStart,
					   extPart->start_sector + offset + extStart + extPart->nr_sectors - 1,
					   extPart->nr_sectors,
					   extPart->nr_sectors * sector_size / 1024,
					   extPart->sys_type); // print partition information

				extPart++; // increment partition
				lseek(fd, (extStart + extPart->start_sector) * sector_size, SEEK_SET); // go to new partition location in disk

 				offset = extPart->start_sector; // update offset variable with the new location of the new sub-partition
			}

			read(fd, buf, sector_size);
		}

		p++;
	}
}