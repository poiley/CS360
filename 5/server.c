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
#include <libgen.h>
#include <errno.h>

#define MAX 256
#define SERVER_HOST "localhost"
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
#define BLKSIZE 4096

struct sockaddr_in server_addr, client_addr;
int mysock, csock;
int r, len, n;
char buf[MAX];
char line[MAX];

struct stat mystat, *sp;
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

char gpath[MAX];
char *cmd;
char *pathname;

int server_init() {
	printf("-----server init start -----\n");
	//create socket
	printf("1.create socket\n");
	mysock = socket(AF_INET, SOCK_STREAM, 0 );
	if(mysock < 0) {
		printf("Socket call failed\n");
		exit(1);
	}

	printf("Fill server_addr with host IP and port info\n");
	server_addr.sin_family = AF_INET; //tcp
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //host ip
	server_addr.sin_port = htons(SERVER_PORT); //port num

	printf("bind socket to server address\n");
	r = bind(mysock, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(r < 0) {
		printf("Bind failed\n");
		exit(3);
	}

	printf("  hostname = %s   port = %d \n", SERVER_HOST, SERVER_PORT);

	//set current directory as root
	getcwd(buf, MAX);
	r = chroot(buf);
	if(r < 0) {
		printf("**error with chroot**\n");
		if(errno == EPERM)
			printf("the caller has insufficient privilege -- try running with sudo for chroot to work\n");
	}
		
	printf("Server is listening ... \n");
	listen(mysock, 5); //queue length == 5
	printf("--- server init done ---- \n");
}

int ls_file(char *fname) { //instead of printing fill up line
	struct stat fstat, *sp;
	int r, i;
	int linknamesize = 128;
	char ftime[64];
	char linkname[128];
	char tempstr[12];

	tempstr[1] = '\0';
	sp = &fstat;
	if((r=lstat(fname, &fstat)) < 0) {
		printf("Can't stat %s\n", fname);
		return -1;
	}

	if((sp->st_mode & 0xF000) == 0x8000) //if is reg
		strcpy(line, "-");
	if((sp->st_mode & 0xF000) == 0x4000) //if is dir
		strcpy(line, "d");
	if((sp->st_mode & 0xF000) == 0xA000) { //if is lnk
		strcpy(line, "l");
		
		if(sp->st_mode & (1<<i)){ //print r|w|x
			tempstr[0] = t1[i];
			strcat(line, tempstr);
		} else {
			tempstr[0] = t2[i];
			strcat(line, tempstr); //or print -
		}
	}
 
	sprintf(tempstr,"%4d ", sp->st_nlink); //link count
	strcat(line, tempstr);
	sprintf(tempstr,"%4d " , sp->st_gid); //gid
	strcat(line, tempstr);
	sprintf(tempstr,"%4d " , sp->st_uid); //uid
	strcat(line, tempstr);
	sprintf(tempstr,"%8d " , sp->st_size); //file size
	strcat(line, tempstr);

	//print time
	strcpy(ftime,ctime(&sp->st_ctime)); //calendar form
	ftime[strlen(ftime) - 1]=0; //kill newline char at end
	strcat(line,ftime);

	//print name
	strcat(line, " ");
	strcat(line, basename(fname));
	
	//print linkname if symbolic file
	if((sp->st_mode & 0xF000) == 0xA000){
		readlink(fname, linkname, linknamesize);
		strcat(line, " -> ");
		strcat(line, linkname);
	}

	strcat(line, "\n");

	printf("%s", line);
	n += write(csock, line, MAX);
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

int findCmd(char *command, char *cmd_list[9]) {
	int i = 0;
	while(cmd_list[i]) {
		if(strcmp(command, cmd_list[i])==0)
			return i;
		i++;
	}
	return -1;
}

int main() {
	char *server_cmds[9] = {"get", "put", "ls", "cd", "pwd", "mkdir", "rmdir", "rm", 0};
	char tempstr[MAX];
	
	server_init();
	while(1) { //try to accept new client request
		printf("Server: accepting new connection...\n");

		len = sizeof(client_addr);
		csock = accept(mysock, (struct sockaddr*)&client_addr, &len);
		if(csock < 0) {
			printf("Server: accept error\n");
			exit(1);
		}

		printf("Server: accepted a client connection from client \n");

		while(1) { //processing loop
			n = read(csock, line, MAX);
			if(n == 0) {
				printf("Server: clinet died, server loops\n");
				close(csock);
				break;
			}

			printf("Server: read %d bytes; line=%s\n", n, line);

			//server deals with request
		 	//seperate command and pathname from line
			strcpy(gpath, line);
			cmd = strtok(gpath, " ");
			pathname = strtok(NULL, " ");
		
			//determine command and execute
			int index = findCmd(cmd, server_cmds);
			char path[1024];
			struct stat mystat, *sp = &mystat;
			int size = 0, total = 0;
			char buf[BLKSIZE];
			int gd;
			switch(index){
				case 0:
					//get
					break;
				case 1:
					//put
					//read number of blk sizes
					n = read(csock, &size, 4);
					if(size == 0){
						printf("No file contents to recieve\n");
						strcpy(line, "No file contents to recieve\n");
					}else{
						//open file for write
						gd = open(pathname, O_WRONLY|O_CREAT,0644);
						if(gd < 0){
							printf("Server: Cannot open file for writing\n");
							strcpy(line, "Cannot open file for writing at server");
						}
						
						while(total < size){
							n = read(csock, buf, BLKSIZE); //reading even if file not open to clear line between client and server
							total += n;
							//put into file if file opened correctly
							if(gd)
								write(gd, buf, n);
						}
						if(gd){
							printf("Server: read %d bytes and put into file %s\n", total+4, pathname);
							strcpy(line, "success");
						}
					}
					//write back results
					n = write(csock, line, MAX);
					printf("Server: wrote additional %d bytes; echo = %s", n, line);
					break;
				case 2:
					//ls
					if(!pathname) //no path/file given use cwd
						strcpy(path, "./");
					else
						strcpy(path, pathname);

					if(r = lstat(path, sp) < 0) {
						sprintf(tempstr,"no such file or directory %s\n", path);
						n=0;
						strcpy(line, tempstr);
						n = write(csock, line, MAX);
						strcpy(line, "END OF ls\n");
						n += write(csock, line, MAX);
						printf("Server: wrote %d bytes\n", n);
						break;
					}
					
					if(path[0] != '/') { //relative, get cwd to make complete path
						getcwd(buf, MAX);
						strcpy(path, buf);
						strcat(path, "/");
						if(pathname)
							strcat(path, pathname);
					}

					n = 0;  
					if((sp->st_mode & 0xF000) == 0x4000) // if its a directory
						ls_dir(path);
					else
						ls_file(path); //if its a file

					strcpy(line, "END OF ls\n");
					n += write(csock, line, MAX);
					printf("Server: wrote %d bytes\n", n);
					break;
				case 3:
					//cd
					r = chdir(pathname);
					if(r < 0)
						strcpy(line, "cd failed");
					else
						strcpy(line, "cd OK");

					n += write(csock, line, MAX);
					printf("Server: wrote %d bytes; echo = %s\n", n,line);
					break;
				case 4:
					//pwd
					getcwd(buf, MAX);
					printf("server cwd: %s\n", buf);
					n = write(csock, buf, MAX);
					printf("server: wrote n=%d bytes; ECHO = %s\n",n,buf);
					break;
				case 5:
					//mkdir
					r = mkdir(pathname, 0755);
					if(r < 0)
						strcpy(line, "mkdir failed");
					else
						strcpy(line, "mkdir OK");

					n += write(csock, line, MAX);
					printf("Server: wrote %d bytes; echo = %s\n", n,line);
					break;
				case 6:
					//rmdir
					r = rmdir(pathname);
					if(r < 0)
						strcpy(line, "rmdir failed");
					else
						strcpy(line, "rmkdir OK");

					n += write(csock, line, MAX);
					printf("Server: wrote %d bytes; echo = %s\n", n,line);
					break;
				case 7:
					//rm
					r = unlink(pathname);
					if(r < 0)
						strcpy(line,"rm failed");
					else
						strcpy(line, "rm OK\n");

					n += write(csock, line, MAX);
					printf("Server: wrote %d bytes; echo = %s\n", n,line);
					break;
				default:
					printf("Command not recognized\n");
					strcpy(line, "command not recognized");
					n =  write(csock, line, MAX);
					printf("server: wrote n=%d bytes; ECHO = %s\n",n,line);
					break;
			}
			printf("Server: ready for next request\n");
		}
	}
}
