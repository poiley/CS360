#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

typedef struct node
{
	char name[64]; 
	char type;	   
	struct node *child, *sibling, *parent;
} NODE;



NODE *root, *cwd;																						 
char line[128];																							 
char command[16], pathname[64];																			 
char dname[64], bname[64];																				 
char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm", "quit", "menu", "reload", "save", 0}; 


void mkdir(void);
void rmdir(void);
void ls(void);
void cd(void);
void pwd(void);
void rpwd(NODE *node);
void creat(void);
void rm(void);
void quit(void);
void menu(void);
void reload(void);
void save(void);
void rsave(FILE *outfile, NODE *node);

void removeNewLine(char *string);
void splitPath(char *path);
void dbname(char *path);

int hasPath(void);
NODE *search_child(char *dname, NODE *child);
NODE *findDir(NODE *root);
void insertNode(NODE *parent, NODE *newNode);
void displayPath(NODE *node);
void memReset(void);
void deleteNode(NODE *node);
void rPrintPath(FILE *outfile, NODE *node);

void mkdir(void) {
	NODE *dir, *newNode; 
	if (!pathname) {
		printf("Invalid path.\n");
		return;
	}

	dbname(pathname);

	if (strcmp(pathname, "") == 0) { 
		printf("No pathname provided.\n");
		return;
	}

	if (pathname[0] == '/')
		dir = root; 
	else			
		dir = findDir(cwd);

	if (dir) { 
		if (search_child(bname, dir->child)) { 
			printf("Directory \'%s\' already exists.\n", bname);
			return;
		}

		newNode = (NODE *) malloc(sizeof(NODE)); 
		strcpy(newNode->name, bname);
		newNode->type = 'D';
		insertNode(dir, newNode);
		return;
	}

	printf("Invalid path.\n");

	return;
}


void insertNode(NODE *parent, NODE *newNode) {
	NODE *navNode; 

	if (parent->child) { 
		navNode = parent->child;
		while (navNode->sibling)
			navNode = navNode->sibling;
		newNode->parent = parent;
		navNode->sibling = newNode;
		return;
	}

	newNode->parent = parent; 
	parent->child = newNode;

	return;
}

void splitPath(char *path) {
	int i = 0, j = 0, baseIndex = 0, dirIndex = 0;

	
	while (path[i] != 0) 
		i++;

	while (path[i] != '/' && i > 0) 
		i--;

	baseIndex = i; 

	if (path[i] == '/')
		i++; 

	while (path[i] != 0) { 
		bname[j] = path[i];
		i++;
		j++;
	}

	bname[j] = 0; 

	while (dirIndex < baseIndex) { 
		dname[dirIndex] = path[dirIndex];
		dirIndex++;
	}

	dname[i] = 0; 

	if (strcmp(dname, "/") == 0)
		return; 

	dirIndex = strlen(dname) - 1; 

	while (dname[dirIndex] != '/' && dirIndex > 0) 
		dirIndex--;

	if (dirIndex != 0) 
		memmove(dname, dname + dirIndex + 1, strlen(dname));

	if (dname[0] == '/') 
		memmove(dname, dname + 1, strlen(dname));

	return;
}

void dbname(char *path) {
	char temp[128];
	strcpy(temp, path);
	strcpy(dname, dirname(temp));
	strcpy(temp, path);
	strcpy(bname, basename(temp));

	return;
}

void rmdir(void) {
	NODE *dir; 

	if (!pathname) {
		printf("Invalid path.\n");
		return;
	}

	dbname(pathname); 

	if (strcmp(pathname, "") == 0) { 
		printf("No pathname provided.\n");
		return;
	}

	if (pathname[0] == '/') 
		dir = findDir(root);
	else 
		dir = findDir(cwd);

	if (dir) { 
		dir = search_child(bname, dir->child); 

		if (!dir) { 
			printf("Directory *%s* does not exist.\n", bname);
			return;
		}

		if (strcmp(dir->name, "/") == 0) { 
			printf("Cannot delete root node.\n");
			return;
		}

		if (dir->type != 'D') { 
			printf("\'%s\' is not a directory.\n", bname);
			return;
		}

		if (dir->child) {
			printf("Directory \'%s\' has contents and cannot be removed.\n", bname);
			return;
		}

		if (strcmp(dir->name, cwd->name) == 0) 
			cwd = cwd->parent;

		deleteNode(dir);

		return;
	}

	printf("Invalid path.\n");

	return;
}


void deleteNode(NODE *node) {
	NODE *parent, *sibling;
	char *childName;

	parent = node->parent;
	childName = parent->child->name;

	if (strcmp(childName, node->name) == 0) { 
		parent->child = node->sibling; 
		free(node);
		return;
	}

	sibling = parent->child; 

	while (strcmp(sibling->sibling->name, node->name) != 0) 
		sibling = sibling->sibling;

	sibling->sibling = node->sibling; 

	free(node); 

	return;
}


void ls(void) {
	NODE *children = cwd->child; 
	int i = 0;

	if (!children) { 
		printf("Directory empty.\n");
		return;
	}

	while (children) { 
		printf("%s\t", children->name); 
		children = children->sibling;
		if (i == 10) { 
			putchar('\n');
			i = 0;
		}
		i++;
	}

	putchar('\n');

	return;
}

void cd(void) {
	NODE *dest;

	if (strcmp(pathname, "") == 0) { 
		cwd = root;
		return;
	}

	if (strcmp(pathname, ".") == 0)
		return; 

	if (strcmp(pathname, "..") == 0) { 
		if (!cwd->parent) {
			printf("No parent directory.\n");
			return;
		}

		cwd = cwd->parent;

		return;
	}

	dbname(pathname); 

	if (pathname[0] == '/') 
		dest = findDir(root);
	else 
		dest = findDir(cwd);

	if (dest) { 
		dest = search_child(bname, dest->child); 

		if (!dest) { 
			printf("Directory does not exist. To create one, enter: mkdir 'directoryname'.\n");
			return;
		}

		if (dest->type != 'D') { 
			printf("Given node is not a directory.\n");
			return;
		}

		cwd = dest; 
		return;
	}

	return;
}

void pwd(void) {
	rpwd(cwd); 
	putchar('\n');

	return;
}


void rpwd(NODE *node) {
	if (!node) 
		return;
	rpwd(node->parent);	  
	printf("%s", node->name); 
	return;
}

void creat(void) {
	NODE *dir, *newNode; 

	if (!pathname) {
		printf("Invalid path.\n");
		return;
	}

	dbname(pathname); 

	if (strcmp(pathname, "") == 0) { 
		printf("No pathname provided.\n");
		return;
	}

	if (pathname[0] == '/') 
		dir = findDir(root);
	else
		dir = findDir(cwd); 

	if (dir) { 
		if (search_child(bname, dir->child)) { 
			printf("File \'%s\' already exists.\n", bname);
			return;
		}

		newNode = (NODE *)malloc(sizeof(NODE)); 
		strcpy(newNode->name, bname);
		newNode->type = 'F';
		insertNode(dir, newNode);
		return;
	}

	printf("Invalid path.\n");

	return;
}

void rm(void) {
	NODE *file, *parent; 

	if (!pathname) {
		printf("Invalid path.\n");
		return;
	}

	dbname(pathname); 

	if (strcmp(pathname, "") == 0) { 
		printf("No pathname provided.\n");
		return;
	}

	if (pathname[0] == '/') 
		file = findDir(root);
	else 
		file = findDir(cwd);

	if (file) { 
		file = search_child(bname, file->child); 

		if (!file) { 
			printf("File\'%s\' does not exist.\n", bname);
			return;
		}

		if (file->type != 'F') { 
			printf("\'%s\' is not a file.\n", bname);
			return;
		}

		deleteNode(file);

		return;
	}

	printf("Invalid path.\n");

	return;
}

void quit(void) {
	save();

	exit(0);
	return;
}

void menu(void) {
	printf("                           Commands Available:\n"
		   "[ mkdir | rmdir | ls | cd | pwd | rm | creat | reload | save | menu | quit ]\n");

	return;
}

void reload(void) {
	FILE *infile;
	char type, trash;

	if (strcmp(pathname, "") == 0)
		return;

	infile = fopen(pathname, "r+");

	if (!infile) {
		printf("Input file not found.\n");
		return;
	}

	fgets(line, 128, infile); 
	fgets(line, 128, infile);
	

	
	fscanf(infile, "%c", &type);	
	fscanf(infile, "%c", &trash);	
	fscanf(infile, "%s", pathname); 
	fscanf(infile, "%c", &trash);	

	do 
	{
		if (feof(infile))
			break;

		memReset(); 

		fscanf(infile, "%c", &type);	
		fscanf(infile, "%c", &trash);	
		fscanf(infile, "%s", pathname); 
		fscanf(infile, "%c", &trash);	

		pathname[strlen(pathname) - 1] = 0; 
											
		if (type == 'D') {
			mkdir();
			continue;
		} else if (type == 'F') {
			creat();
			continue;
		}

	} while (type && pathname);

	return;
}

void save(void) {
	FILE *outfile;

	if (strcmp(pathname, "") == 0)
		return;

	outfile = fopen(pathname, "w+");

	fprintf(outfile, "Type\tPath\n");
	fprintf(outfile, "-----  -----------------\n");

	rsave(outfile, root); 

	fclose(outfile);

	return;
}

void rsave(FILE *outfile, NODE *node) {
	NODE *child;

	if (!node)
		return;

	fprintf(outfile, "%c\t", node->type);

	rPrintPath(outfile, node);
	fprintf(outfile, "\n");

	child = node->child;

	while (child) {
		rsave(outfile, child);
		child = child->sibling;
	}

	return;
}

void rPrintPath(FILE *outfile, NODE *node) {
	
	if (node->parent)
		rPrintPath(outfile, node->parent);

	fprintf(outfile, "%s", node->name);
	if (strcmp(node->name, "/") != 0 && strcmp(node->name, root->name) != 0) {
		fprintf(outfile, "/");
	} 
}




int findCmd(char *command) {
	int i = 0;

	while (cmd[i]) {
		if (strcmp(command, cmd[i]) == 0)
			return i;
		i++;
	}

	return -1;
}


NODE *findDir(NODE *node) {
	char *token, *dirPath; 
	int i = 1, j = 0;

	if (strcmp(dname, ".") == 0)
		return node; 

	if (hasPath()) { 
		dirPath = malloc(sizeof(char) * strlen(pathname));
		strcpy(dirPath, pathname); 
		j = strlen(dirPath) - 1;

		while (dirPath[j] != '/')
			j--; 
		dirPath[j] = 0;

		token = strtok(dirPath, "/");

		do { 
			node = search_child(token, node->child); 
			if (node) {								
				if (node->type != 'D') {
					printf("Node \'%s\' is not a directory.\n", token);
					return 0;
				}
			}
			else
				return 0;
			i++;
		} while (token = strtok(0, "/"));
	}

	if (strcmp(node->name, dname) != 0)
		return 0; 

	return node;
}


NODE *search_child(char *dname, NODE *child) {
	while (child) { 
		if (strcmp(child->name, dname) == 0) 
			return child;
		child = child->sibling;
	}
	return 0;
}


void initialize() {
	root = malloc(sizeof(NODE));
	strcpy(root->name, "/");
	root->type = 'D';
	root->child = root->sibling = root->parent = NULL;

	cwd = root;
	return;
}


void displayPath(NODE *node) {
	
	if (node->parent)
		displayPath(node->parent);

	printf("%s", node->name);
	if (strcmp(node->name, "/") != 0 && strcmp(node->name, cwd->name) != 0) 
		putchar('/');
}


int hasPath(void) {
	int i = 0;

	while (pathname[i]) {
		if (pathname[i] == '/')
			return 1;
		i++;
	}

	return 0;
}

void memReset(void) {
	memset(line, 0, 128);
	memset(command, 0, 16);
	memset(pathname, 0, 64);
	memset(dname, 0, 64);
	memset(bname, 0, 64);

	return;
}


int main(void) {
	char *stemp;

	
	initialize();
	

	
	while (1) {
		
		memReset();

		
		displayPath(cwd);
		printf("\nCommand :");
		fgets(line, 128, stdin);	
		line[strlen(line) - 1] = 0; 
		sscanf(line, "%s %s", command, pathname);
		int id = findCmd(command);
		
		
		switch (id) {
			case 0:
				mkdir();
				break;
			case 1:
				rmdir();
				break;
			case 2:
				ls();
				break;
			case 3:
				cd();
				break;
			case 4:
				pwd();
				break;
			case 5:
				creat();
				break;
			case 6:
				rm();
				break;
			case 7:
				strcpy(pathname, "backup.txt");
				save();
				quit();
				break;
			case 8:
				menu();
				break;
			case 9:
				reload();
				break;
			case 10:
				save();
				break;
			default:
				printf("Command \' %s\'  not found.\n", command);
				break;
		}

		putchar('\n');
	}
}
