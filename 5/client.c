#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <libgen.h> //need this for POSIX version of basname

#define MAX 256
#define SERVER_HOST "localhost"
#define SERVER_PORT 1234
#define BLKSIZE 4096
struct sockaddr_in server_addr;
int sock, r;

char gpath[MAX];
char *cmd;
char *pathname;

struct stat mystat, *sp;
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

int client_init() {
	printf("---- client init start -----\n");

	printf("create tcp socket\n");
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		printf("socket call failed\n");
		exit(1);
  	}

	printf("fill ser_addr with servers IP and port#\n");
	server_addr.sin_family= AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVER_PORT);

	printf("connecting to server ... \n");
	r= connect(sock, (struct sockaddr*) &server_addr, sizeof(server_addr));
	if(r < 0) {
		printf("connect failed\n");
		exit(3);
	}

	printf("connected OK to server\n");
	printf("\t hostname=%s, PORT=%d\n", SERVER_HOST, SERVER_PORT);

	printf("-----Client init done---\n");
}

void print_options() {
	printf("*********************menu****************\n");
	printf("* get put ls  cd  pwd  mkdir  rmdir  rm *\n");
	printf("* lcat   lls lcd lpwd lmkdir lrmdir lrm *\n");
	printf("* enter an empty line to exit           *\n");
	printf("*****************************************\n");
}

int findCmd(char *command, char *cmd_list[8]) {
	int i = 0;
	while(cmd_list[i]) {
		if(strcmp(command, cmd_list[i])==0)
			return i;
		i++;
	}
	return -1;
}

int mycat(char *filename) {
	int fd, n, m;
	char buf[BLKSIZE];
	fd = -1;

	if(filename) {
		fd = open(filename, O_RDONLY);
		if(fd< 0)
			return -2;
	} else {
		return -1;
	}

	//opened correctly, read and ouput in blksize chunks
	while(n = read(fd, buf, BLKSIZE))
		m = write(1, buf, n);
}

int ls_file(char *fname) {
	struct stat fstat, *sp;
	int r, i;
	char ftime[64];
	char linkname[128];
	int linknamesize=128;
	sp = &fstat;

	if((r = lstat(fname, &fstat)) < 0) {
		printf("Can't stat %s\n", fname);
		return -1;
	}

	if((sp->st_mode & 0xF000) == 0x8000) //if is reg
		printf("%c", '-');
	if((sp->st_mode & 0xF000) == 0x4000) //if is dir
		printf("%c", 'd');
	if((sp->st_mode & 0xF000) == 0xA000) //if is lnk
		printf("%c", 'l');

	for(i = 8; i >= 0; i--) {
		if(sp->st_mode & (1<<i)) //print r|w|x
			printf("%c", t1[i]);
		else
			printf("%c", t2[i]); //or print -
	}
	
	printf("%4d ", sp->st_nlink); //link count
	printf("%4d " , sp->st_gid); //gid
	printf("%4d " , sp->st_uid); //uid
	printf("%8d " , sp->st_size); //file size

	//print time
	strcpy(ftime, ctime(&sp->st_ctime)); //calendar form
	ftime[strlen(ftime) - 1] = 0; //kill newline char at end
	printf("%s  ", ftime);
	
	//print name
	printf("%s", basename(fname));
	
	//print linkname if symbolic file
	if((sp->st_mode & 0xF000)==0xA000) {
		readlink(fname, linkname, linknamesize);
		printf(" -> %s", linkname);
	}

	printf("\n");
}

int ls_dir(char *dirname) {
	DIR *dp = opendir(dirname);
	if(dp) {
		struct dirent* next = readdir(dp);
		while(next) {
			ls_file(next->d_name);
			next = readdir(dp);
		}
	} else {
		return -1;
	}
}


int main() {
	int n;
	char line[MAX], ans[MAX];
	char *local_cmds[8] = {"lcat", "lls", "lcd", "lpwd", "lmkdir", "lrmdir", "lrm", 0};
	
	client_init();
	print_options();

	printf("====processing loop====\n");
	while(1) {
		printf("Enter a command: ");
		bzero(line, MAX); // zero out line[]
		fgets(line, MAX, stdin); // get a line
		line[strlen(line) - 1] = 0; // kill newline
		if(line[0] == 0) // exit if null line
			exit(0);

		// tokenize first word off line
		strcpy(gpath, line);
		cmd = strtok(gpath, " ");
		pathname = strtok(NULL, " ");

		int index = findCmd(cmd, local_cmds);
		if(index == -1) {
			// not local, send to server
			n = write(sock, line, MAX);
			printf("Client: wrote %d bytes; line = %s\n", n, line);

			if(strcmp(cmd, "ls") == 0) { // ls reads lines until sees "END OF ls"
	 			n = 0;
				do {
					n += read(sock, line, MAX);
					printf("%s", line);
	 			} while(strcmp(line, "END OF ls\n") != 0);

	 			printf("Client read %d bytes\n", n);
			} else if(strcmp(cmd, "get") == 0) { // recives file size first, then waits to read that many bytes
				printf("Get command Run now!");
			} else if(strcmp(cmd, "put") == 0) {//send file size first so server knows how long to read
				int fd; //try to open file for read
				int size = 0;
				fd = open(pathname, O_RDONLY);
				if(fd < 0) {
					printf("Client: cannot open file %s for read\n", pathname);
					n = write(sock, &size, 4); //write size 0 so know nothing to recieve
					break;
				}

				//get file size
				struct stat filestat, *sp;
				sp = &filestat;
				if((r=fstat(fd, sp)) < 0) {
					printf("cannot stat file\n");
				}

				int total = 0;
				size = sp->st_size; //its off_t but use int ok?
				//round size up to nearest blksize
						
				//send # bytes to be sent
				total = write(sock, &size, 4);
				n = 0;
				char buf[BLKSIZE];

				//read and write file tell end in blocksize chunks
				while((n = read(fd, buf, BLKSIZE)) > 0) {
					write(sock, buf, n);  
					total +=n;
				}

				printf("Client: wrote %d bytes\n",total);
				//get server response
				n = read(sock, line, MAX);
				printf("server says:  %s\n", line);
			} else { //only one line to read
				n = read(sock, line, MAX);
				printf("Client: read %d bytes; echo =%s\n", n, line);
			}
		} else {
			//is local, execute locally
			char buf[MAX];
			char path[1024];
			struct stat mystat, *sp = &mystat;
			switch(index) {
				case 0:
					//lcat
					r = mycat(pathname);
					if(r == -1)
						printf("lcat failed, no filename\n");
					else if(r == -2)
						printf("lcat failed, problem opening file\n");
					break;
				case 1:
					//lls
					if(!pathname) {
						//no path/file given use cwd
						strcpy(path, "./");
					} else {
						strcpy(path, pathname);
					}
					
					if(r=lstat(path, sp) < 0) {
						printf("no such file or directory %s\n", path);
						break;
					}
					
					if(path[0] != '/') { //relative, get cwd to make complete path
						getcwd(buf, MAX);
						strcpy(path, buf);
						strcat(path, "/");
						if(pathname) {
							strcat(path, pathname);
						}
					}
						
					if((sp->st_mode & 0xF000) == 0x4000) // if its a directory
						ls_dir(path);
					else
						ls_file(path); //if its a file
					break;
				case 2:
					//lcd
					r = chdir(pathname);
					if(r < 0)
						printf("\t lcd failed\n");
					else
						printf("\t lcd OK\n");
					break;
				case 3:
					//lpwd
					getcwd(buf, MAX);
					printf("local cwd: %s\n", buf);
					break;
				case 4:
					//lmkdir
					r = mkdir(pathname, 0755);
					if(r< 0)
						printf("\t lmkdir failed\n");
					else
						printf("\t lmkdir OK\n");
					break;
				case 5:
					//lrmdir
					r = rmdir(pathname);
					if(r < 0)
						printf("\t lrmdir failed\n");
					else
						printf("\t lrmdir OK\n");
					break;
				case 6:
					//lrm
					r = unlink(pathname);
					if(r < 0)
						printf("\t lrm failed\n");
					else
						printf("\t lrm OK\n");
					break;
			}
		}
	}
}
