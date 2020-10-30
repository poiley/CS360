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
int sock, csock, r, len, n;

char gpath[MAX], buf[MAX], line[MAX];
char *cmd;
char *pathname;

struct stat mystat, *sp;
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";


int server_init() {
	printf("----- SERVER Initialization -----\n");
	
	printf("SERVER: Creating TCP socket.\n");
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		printf("** SERVER ERROR: Socket call failed in server_init()\n");
		exit(1);
	}

	printf("SERVER: Populating server_addr attributes in server_init()\n");
	server_addr.sin_family = AF_INET; 
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	server_addr.sin_port = htons(SERVER_PORT); 

	printf("SERVER: Binding socket to server address.\n");
	r = bind(sock, (struct sockaddr*) &server_addr, sizeof(server_addr));
	if(r < 0) {
		printf("** SERVER ERROR: Socket bind failed in server_init()\n");
		exit(3);
	}
	
	getcwd(buf, MAX);
	r = chroot(buf);
	if(r < 0) {
		printf("** SERVER ERROR: Command 'chroot': FAILED");
		if(errno == EPERM)
			printf(" due to insufficient privileges -- perhaps try running as root?");
		putchar('\n');
	}

	printf("SERVER: Now listening on %s:%d\n", SERVER_IP, SERVER_PORT);
	listen(sock, 5); 
	printf("--- SERVER Initialization SUCCESSFUL ---- \n");
}


int ls_file(char *fname) { 
	struct stat fstat, *sp;
	int r, i;
	int linknamesize = 128;
	char ftime[64];
	char linkname[128];
	char tempstr[12];

	tempstr[1] = '\0';
	sp = &fstat;
	if((r = lstat(fname, &fstat)) < 0) {
		printf("** SERVER ERROR: Command 'ls': Can't get attributes of '%s'\n", fname);
		return -1;
	}

	if((sp->st_mode & 0xF000) == 0x8000) 
		strcpy(line, "-");
	if((sp->st_mode & 0xF000) == 0x4000) 
		strcpy(line, "d");
	if((sp->st_mode & 0xF000) == 0xA000) { 
		strcpy(line, "l");
		
		if(sp->st_mode & (1<<i)){ 
			tempstr[0] = t1[i];
			strcat(line, tempstr);
		} else {
			tempstr[0] = t2[i];
			strcat(line, tempstr); 
		}
	}
 
	sprintf(tempstr,"%4d ", sp->st_nlink); 
	strcat(line, tempstr);
	sprintf(tempstr,"%4d " , sp->st_gid); 
	strcat(line, tempstr);
	sprintf(tempstr,"%4d " , sp->st_uid); 
	strcat(line, tempstr);
	sprintf(tempstr,"%8ld " , sp->st_size); 
	strcat(line, tempstr);
	
	strcpy(ftime,ctime(&sp->st_ctime)); 
	ftime[strlen(ftime) - 1]=0; 
	strcat(line,ftime);
	
	strcat(line, " ");
	strcat(line, basename(fname));
	
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
	while(1) { 
		printf("SERVER: Accepting new connections...\n");

		len = sizeof(client_addr);
		csock = accept(sock, (struct sockaddr*)&client_addr, &len);
		if(csock < 0) {
			printf("** SERVER ERROR: Failure while establishing connection to client, can't accept socket.\n");
			exit(1);
		}

		printf("SERVER: Client has connected.\n");

		while(1) { 
			n = read(csock, line, MAX);
			if(n == 0) {
				printf("SERVER: Client has disconnected.\n");
				close(csock);
				break;
			}

			printf("SERVER: Read %d bytes; line = '%s'\n", n, line);

			strcpy(gpath, line);
			cmd = strtok(gpath, " ");
			pathname = strtok(NULL, " ");		
			
			char buf[BLKSIZE], path[1024];
			struct stat mystat, *sp = &mystat;
			int index = findCmd(cmd, server_cmds), size = 0, total = 0, gd;
			switch(index){
				case 0:
					printf("SERVER: Command 'get': Executing '%s' in '%s'\n", cmd, path);
					int fd = open(path, O_RDONLY);
					if (fd >= 0) { 
						lstat(path, &mystat);
						sprintf(buf, "%ld", mystat.st_size);
						write(csock, buf, sizeof(buf));         
						bzero(buf, sizeof(buf)); buf[sizeof(buf) - 1] = '\0';

						int bytes = 0;
						while (n = read(fd, buf, sizeof(buf))) { 
							if (n != 0) {
								buf[sizeof(buf) - 1] = '\0';
								
								bytes += n;
								printf("SERVER: Command 'get': Wrote %d bytes.\n", bytes);

								write(csock, buf, sizeof(buf));
								bzero(buf, sizeof(buf));
								buf[sizeof(buf) - 1] = '\0';
							}
						} close(fd);
					} else
						write(csock, "BAD", sizeof("BAD"));
					break;
				case 1:
					n = read(csock, &size, 4);
					if(size == 0) {
						printf("** SERVER ERROR: Command 'put': No file contents to recieve.\n");
						strcpy(line, "No file contents to recieve\n");
					} else {
						gd = open(pathname, O_WRONLY|O_CREAT,0644);
						if(gd < 0) {
							printf("** SERVER ERROR: Command 'put': Cannot open file for writing.\n");
							strcpy(line, "Cannot open file for writing on SERVER");
						}
						
						while(total < size) {
							n = read(csock, buf, BLKSIZE); 
							total += n;
							
							if(gd)
								write(gd, buf, n);
						}

						if(gd) {
							printf("SERVER: Command 'put': Wrote %d bytes to %s\n", total+4, pathname);
							strcpy(line, "success");
						}
					}
					
					n = write(csock, line, MAX);
					printf("SERVER: Command 'put': Wrote additional %d bytes; echo = '%s'", n, line);
					break;
				case 2:
					if(!pathname) 
						strcpy(path, "./");
					else
						strcpy(path, pathname);

					if(r = lstat(path, sp) < 0) {
						sprintf(tempstr, "SERVER: Command 'ls': No such file or directory\n", path);
						n=0;
						strcpy(line, tempstr);
						n = write(csock, line, MAX);
						strcpy(line, "END OF ls\n");
						n += write(csock, line, MAX);
						printf("SERVER: Command 'ls': Wrote %d bytes.\n", n);
						break;
					}
					
					if(path[0] != '/') { 
						getcwd(buf, MAX);
						strcpy(path, buf);
						strcat(path, "/");
						if(pathname)
							strcat(path, pathname);
					}

					n = 0;  
					if((sp->st_mode & 0xF000) == 0x4000) 
						ls_dir(path);
					else
						ls_file(path); 

					strcpy(line, "END OF ls\n");
					n += write(csock, line, MAX);
					printf("SERVER: Command 'ls': Wrote %d bytes.\n", n);
					break;
				case 3:
					r = chdir(pathname);
					if(r < 0)
						strcpy(line, "SERVER: Command 'cd': FAILED");
					else
						strcpy(line, "SERVER: Command 'cd': SUCCESS");

					n += write(csock, line, MAX);
					printf("SERVER: Command 'cd': Wrote %d bytes; echo = '%s'\n", n, line);
					break;
				case 4:
					getcwd(buf, MAX);
					printf("SERVER: Command 'cwd': Current Working Directory: '%s'\n", buf);
					n = write(csock, buf, MAX);
					printf("SERVER: Wrote %d bytes; echo = '%s'\n", n, buf);
					break;
				case 5:
					r = mkdir(pathname, 0755);

					if(r < 0)
						strcpy(line, "SERVER: Command 'mkdir': FAILED");
					else
						strcpy(line, "SERVER: Command 'mkdir': SUCCESS");

					n += write(csock, line, MAX);
					printf("SERVER: Command 'mkdir': Wrote %d bytes; echo = '%s'\n", n, line);
					break;
				case 6:
					r = rmdir(pathname);
					if(r < 0)
						strcpy(line, "SERVER: Command 'rmdir': FAILED");
					else
						strcpy(line, "SERVER: Command 'rmdir': SUCCESS");

					n += write(csock, line, MAX);
					printf("SERVER: Command 'rmdir': Wrote %d bytes; echo = '%s'\n", n, line);
					break;
				case 7:
					r = unlink(pathname);
					if(r < 0)
						strcpy(line, "SERVER: Command 'rm': FAILED");
					else
						strcpy(line, "SERVER: Command 'rm': SUCCESS\n");

					n += write(csock, line, MAX);
					printf("SERVER: Command 'rm': Wrote %d bytes; echo = '%s'\n", n, line);
					break;
				default:
					printf("** SERVER ERROR: Invalid Command\n");
					strcpy(line, "** ERROR: Invalid Command");
					n =  write(csock, line, MAX);
					printf("SERVER: Default: Wrote %d bytes; echo = '%s'\n", n, line);
					break;
			}
			printf("SERVER: Ready for next request.\n");
		}
	}
}
