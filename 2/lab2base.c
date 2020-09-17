#include <stdio.h>             // for I/O
#include <stdlib.h>            // for lib functions
#include <libgen.h>            // for dirname()/basename()
#include <string.h>

typedef struct node{
	char  name[64];       	// node's name string
	char  type;				// possible types: 'D' for directory, 'F' for file
	struct node *child, *sibling, *parent;
} NODE;

NODE *root, *cwd, *start;
char command[16], pathname[64];

char gpath[128];               // global gpath[] to hold token strings
char *name[64];                // token string pointers
int  n;                        // number of token strings

char dname[64], bname[64];     // dirname, basename of pathname

//                0        1      2     3      4       5      6       7        8       9       10 
char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm", "reload", "save", "menu", "quit", NULL};

int findCmd(char *command) {
	for(int i = 0; cmd[i]; i++)
		if (strcmp(command, cmd[i]) == 0)
			return i;
	return -1;
}

NODE *search_child(NODE *parent, char *name) {
	NODE *p;
	printf("search for %s in parent DIR\n", name);
	p = parent->child;
	if (p == 0)
		return 0;
	while(p) {
		if (strcmp(p->name, name) == 0)
			return p;
		p = p->sibling;
	}
	return 0;
}

int insert_child(NODE *parent, NODE *q) {
	NODE *p;
	printf("insert NODE %s into parent child list\n", q->name);
	p = parent->child;
	if (p==0)
		parent->child = q;
	else {
		while(p->sibling)
			p = p->sibling;
		p->sibling = q;
	}
	q->parent = parent;
	q->child = 0;
	q->sibling = 0;
}

/***************************************************************
 This mkdir(char *name) makes a DIR in the current directory
 You MUST improve it to mkdir(char *pathname) for ANY pathname
****************************************************************/
int mkdir(char *pathname) {
	NODE *p, *q;
	printf("mkdir: name=%s\n", pathname);

	// Do not allow mkdir of /, ., ./, .., or ../
	switch(pathname) {
		case "/":
		case ".":
		case "./":
		case "..":
		case "../":
			printf("name %s is invalid. mkdir FAILED\n". pathname);
			return -1;
	}

	if (pathname[0] == '/')
		start = root;
	else
		start = cwd;

	printf("check whether %s already exists\n", pathname);
	p = search_child(start, pathname);

	if (p) {
		printf("name %s already exists, mkdir FAILED\n", pathname);
		return -1;
	}

	printf("--------------------------------------\n");
	printf("ready to mkdir %s\n", pathname);
	q = (NODE *) malloc(sizeof(NODE));
	q->type = 'D';
	strcpy(q->name, pathname);
	insert_child(start, q);
	printf("mkdir %s OK\n", pathname);
	printf("--------------------------------------\n");
		
	return 1;
}

// This ls() list CWD. You MUST improve it to ls(char *pathname)
int ls(char *pathname) {
	NODE *p = cwd->child;
	printf("cwd contents = ");
	while (p) {
		printf("[%c %s] ", p->type, p->name);
		p = p->sibling;
	}
	printf("\n");
}

int cd(char *pathname) {}

int pwd() {}

int creat(char *pathname) {}

int rm(char *pathname) {}

int reload(char *pathname) {}

int save() {}

int menu() {}

int quit() {
	printf("Program exit\n");
	exit(0);
	
	// improve quit() to SAVE the current tree as a Linux file
	// for reload the file to reconstruct the original tree
}

int initialize() { // create / node, set root and cwd pointers
	root = (NODE *)malloc(sizeof(NODE));
	strcpy(root->name, "/");
	root->parent = root;
	root->sibling = 0;
	root->child = 0;
	root->type = 'D';
	cwd = root;
	printf("Root initialized OK\n");
}

int main() {
	int index;
	char line[128];
	
	initialize();

	printf("NOTE: commands = [mkdir|rmdir|ls|cd|pwd|creat|rm|reload|save|menu|quit]\n");

	while(1) {
		printf("Enter command line : ");
		fgets(line, 128, stdin);
		line[strlen(line)-1] = 0;

		sscanf(line, "%s %s", command, pathname);
		printf("command=%s pathname=%s\n", command, pathname);
		
		if (command[0] == 0) 
			continue;

		index = findCmd(command);

		switch (index) {
			case 0: mkdir(pathname); 	break;
			case 1: rmdir(pathname);   	break;
			case 2: ls(pathname);		break;
			case 3: cd(pathname);		break;
			case 4: pwd();			break;
			case 5: creat(pathname);	break;
			case 6: rm(pathname);		break;
			case 7: reload(pathname);	break;
			case 8: save();			break;
			case 9: menu();			break;
			case 10:quit();			break;
		}
  	}
}



int tokenize(char *pathname) {
	// Pseudocode:
	// Divide pathname into token strings in gpath[128]
	// Let char * name[0], name[1], ..., name[n-1] point at token strings
	// Let n = number of token strings
	// print out token strings to verify

	char *s;
	s = strtok(pathname, "/"); // first call to strtok

	while(s) {
		printf("%s ", s);
		s = strtok(0, "/"); // call strtok() until it returns NULL
	}

}

NODE *path2node(char *pathname) {
   // return pointer to the node of pathname, or NULL if invalid
   if (pathname[0] == "/")
	  start = root;
   else
	  start = cwd;

   tokenize(pathname);
   NODE *node = start;

   for (int i=0; i<n; i++) {
	node = search_child(node, name[i]);
	if (!node || node == 0)
		return NULL;
   }

   return node;
}

int dir_base_name(char *pathname) {
	char temp[128];
	strcpy(temp, pathname);
	strcpy(dname, dirname(temp));
	strcpy(temp, pathname);
	strcpy(bname, basename(temp));
}
