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

#define MAX 256
#define SERVER_HOST "localhost"
#define SERVER_PORT 1234
#define BLKSIZE 4096

struct sockaddr_in server_addr;
int sock, r, size = 0;

char gpath[MAX];
char *cmd;
char *pathname;

struct stat mystat, *sp;
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";


int client_init() {
	printf("----- CLIENT Initialization -----\n");

	printf("CLIENT: Create TCP socket.\n");
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		printf("** CLIENT ERROR: Socket call failed.\n");
		exit(1);
  	}

	printf("CLIENT: Populating server_addr attributes in client_init()\n");
	server_addr.sin_family= AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVER_PORT);

	printf("CLIENT: Connecting to %s:%d.\n", SERVER_HOST, SERVER_PORT);
	r = connect(sock, (struct sockaddr*) &server_addr, sizeof(server_addr));
	if(r < 0) {
		printf("** CLIENT ERROR: Failed to connect to %s:%d in client_init()\n", SERVER_HOST, SERVER_PORT);
		exit(3);
	}

	printf("CLIENT: Successfully connected to the server.\n");

	printf("----- CLIENT Initialization SUCCESSFUL ----- \n");
}


void print_options() {
	printf("************************* COMMANDS *************************\n");
	printf("* get\tput\tls\tcd\tpwd\tmkdir\trmdir\trm *\n");
	printf("* lcat\tlls\tlcd\tlpwd\tlmkdir\tlrmdir\tlrm\t   *\n");
	printf("* \t\tPress Return twice to exit\t\t   *\n");
	printf("************************************************************\n");
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
		printf("** CLIENT ERROR: Command 'ls': Can't get attributes of '%s'\n", fname);
		return -1;
	}

	if((sp->st_mode & 0xF000) == 0x8000)
		printf("%c", '-');
	if((sp->st_mode & 0xF000) == 0x4000)
		printf("%c", 'd');
	if((sp->st_mode & 0xF000) == 0xA000)
		printf("%c", 'l');

	for(i = 8; i >= 0; i--) {
		if(sp->st_mode & (1<<i))
			printf("%c", t1[i]);
		else
			printf("%c", t2[i]);
	}

	printf("%4d ", sp->st_nlink);
	printf("%4d " , sp->st_gid);
	printf("%4d " , sp->st_uid);
	printf("%8ld " , sp->st_size);

	strcpy(ftime, ctime(&sp->st_ctime));
	ftime[strlen(ftime) - 1] = 0;
	printf("%s  ", ftime);

	printf("%s", basename(fname));

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
	char *local_cmds[8] = {"lcat", "lls", "lcd", "lpwd", "lmkdir", "lrmdir", "lrm", 0}, *token;

	client_init();
	print_options();

	while(1) {
		getcwd(line, MAX);
		printf("Client@Lab5 : %s $ ", line);
		bzero(line, MAX);
		fgets(line, MAX, stdin);
		line[strlen(line) - 1] = 0;
		if(line[0] == 0)
			exit(0);

		strcpy(gpath, line);
		cmd = strtok(gpath, " ");
		pathname = strtok(NULL, " ");

		int index = findCmd(cmd, local_cmds);
		if(index == -1) {
			n = write(sock, line, MAX);
			printf("CLIENT: Wrote %d bytes; line = '%s'\n", n, line);

			if(strcmp(cmd, "ls") == 0) {
	 			n = 0;
				do {
					n += read(sock, line, MAX);
					printf("%s", line);
	 			} while(strcmp(line, "END OF ls\n") != 0);

	 			printf("CLIENT: Read %d bytes.\n", n);
			} else if(strcmp(cmd, "get") == 0) {
				token = strtok(gpath, "/");
				while (token != NULL) {
					strcpy(cmd, token);
					token = strtok(NULL, "/");
				}

				write(sock, line, sizeof(line));
				printf("CLIENT: Command 'get': Wrote '%s' to SERVER.\n", line);

				char buf[MAX];
				read(sock, buf, sizeof(buf));
				printf("CLIENT: Command 'get': Read back '%s' from SERVER.\n", buf);
				buf[sizeof(buf) - 1] = '\0';

				if (strcmp(buf, "BAD") != 0) {
					size = atoi(buf);
					int fd = open(pathname, O_WRONLY|O_CREAT, 0644);
					int bytes = 0;

					printf("CLIENT: Expecting %d bytes.\n", size);

					while (bytes < size) {
						n = read(sock, buf, sizeof(buf));
						bytes += n;
						printf("CLIENT: Command 'get': Read %d bytes.\n", n);
						write(fd, buf, sizeof(buf));
						bzero(buf, sizeof(buf)); buf[sizeof(buf) - 1] = '\0';
					}
				} else
					printf("** CLIENT ERROR: Command 'get': File '%s' not found on server", pathname);
				putchar('\n');
			} else if(strcmp(cmd, "put") == 0) {
				int fd;
				int size = 0;
				fd = open(pathname, O_RDONLY);
				if(fd < 0) {
					printf("** CLIENT ERROR: Command 'put': Cannot read from file %s\n", pathname);
					n = write(sock, &size, 4);
					break;
				}

				struct stat filestat, *sp;
				sp = &filestat;
				if((r = fstat(fd, sp)) < 0) {
					printf("** CLIENT ERROR: Command 'put': Cannot get statistics for file\n");
				}

				int total = 0;
				size = sp->st_size;

				total = write(sock, &size, 4);
				n = 0;
				char buf[BLKSIZE];

				while((n = read(fd, buf, BLKSIZE)) > 0) {
					write(sock, buf, n);
					total +=n;
				}

				printf("CLIENT: Command 'put': Wrote %d bytes.\n",total);

				n = read(sock, line, MAX);
				printf("SERVER response:  '%s'\n", line);
			} else {
				n = read(sock, line, MAX);
				printf("CLIENT: Read %d bytes; echo = '%s'\n", n, line);
			}
		} else {
			char buf[MAX];
			char path[1024];
			struct stat mystat, *sp = &mystat;
			switch(index) {
				case 0:

					r = mycat(pathname);
					if(r == -1)
						printf("** CLIENT ERROR: Command 'lcat': No filename.\n");
					else if(r == -2)
						printf("** CLIENT ERROR: Command 'lcat': Cannot open open file.\n");
					break;
				case 1:
					if(!pathname) {

						strcpy(path, "./");
					} else {
						strcpy(path, pathname);
					}

					if(r=lstat(path, sp) < 0) {
						printf("** CLIENT ERROR: Command 'lls': '%s' is not a valid file or directory.\n", path);
						break;
					}

					if(path[0] != '/') {
						getcwd(buf, MAX);
						strcpy(path, buf);
						strcat(path, "/");
						if(pathname)
							strcat(path, pathname);
					}

					if((sp->st_mode & 0xF000) == 0x4000)
						ls_dir(path);
					else
						ls_file(path);
					break;
				case 2:
					r = chdir(pathname);
					if(r < 0)
						printf("** CLIENT ERROR: Command 'lcd': FAILED\n");
					else
						printf("CLIENT: Command 'lcd': SUCCESS\n");
					break;
				case 3:
					getcwd(buf, MAX);
					printf("CLIENT: Command 'cwd': Current Working Directory: '%s'\n", buf);
					break;
				case 4:
					r = mkdir(pathname, 0755);
					if(r < 0)
						printf("CLIENT: Command 'lmkdir': FAILED\n");
					else
						printf("CLIENT: Command 'lmkdir': SUCCESS\n");
					break;
				case 5:
					r = rmdir(pathname);
					if(r < 0)
						printf("CLIENT: Command 'lrmdir': FAILED\n");
					else
						printf("CLIENT: Command 'lrmdir': SUCCESS\n");
					break;
				case 6:
					r = unlink(pathname);
					if(r < 0)
						printf("CLIENT: Command 'lrm': FAILED\n");
					else
						printf("CLIENT: Command 'lrm': SUCCESS\n");
					break;
			}
		}
	}
}
