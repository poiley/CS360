void myprintf(char *fmt, ...);

typedef unsigned int u32;

char *ctable = "0123456789ABCDEF";

/*
Function:   rpu
Purpose:    Recursive function to handle unsigned integers of any base
*/
int rpu(u32 x, int BASE) {
    char c;
    if (x) {
        c = ctable[x % BASE];
        rpu(x / BASE, BASE);
        putchar(c);
    }
}

/*
Function:   printu
Purpose:    Prints unsigned integers with parameters for custom bases
*/
int printu(u32 x, int BASE) {
    (x == 0) ? putchar('0') : rpu(x, BASE);
    putchar(' ');
}

/*
Function:   prints
Purpose:    Prints Strings
*/
char *prints(char *s) {
    for (int i = 0; s[i] != '\0'; i++)
        putchar(s[i]);
}

/*
Function:   printd
Purpose:    Prints Integer values of base 10
*/
int printd(int x) {
    if(x < 0) {
        putchar('-');
        x *= -1;
    }

    printu(x, 10);
}

/*
Function:   printx
Purpose:    Prints hexadecimal values
*/
int printx(u32 x) {
    myprintf("0x");
    printu((u32) x, 16);
}

/*
Function:   printo
Purpose:    Prints Octal Values
*/
int printo(u32 x) {
    putchar('0');
    printu((u32) x, 8);
}

/*
Function:   myprintf
Purpose:    Handles the input of certain variable types and prints their output
*/
void myprintf(char *fmt, ...) {
    char *cp = fmt;
    int *ip = (int *) &fmt + 1;

    while (*cp) {
        if (*cp != '%')
            putchar(*cp);
        else
            switch (*(cp + 1)) {
                case 'c':
                    putchar((char) *ip++);
                    cp++;
                    break;
                case 's':
                    prints((char *) *ip++);
                    cp++;
                    break;
                case 'u':
                    printu((u32) *ip++, 10);
                    cp++;
                    break;
                case 'd':
                    printd((int) *ip++);
                    cp++;
                    break;
                case 'o':
                    printo((u32) *ip++);
                    cp++;
                    break;
                case 'x':
                    printx((u32) *ip++);
                    cp++;
                    break;
                default:
                    putchar(*fmt);
                    break;
            }
        cp++;
    }
}

/*
Function:   testmyprintf
Purpose:    To test the functionality of each portion of myprintf
*/
void testmyprintf() {
    myprintf("\nThis is my function to test myprintf\n");
    myprintf("This is the number %d\n", 42);
    myprintf("This is the same number in hex, octal, and as a negative: %x %o %d\n", 42, 42, -42);
    myprintf("Here's the first letter of the alphabet: %c\n", 'a');
    myprintf("Here is a sentence: %s\n", "In the beginning the Universe was created. This has made a lot of people very angry and been widely regarded as a bad move.");
}

int main(int argc, char *argv[], char *env[]) {
    myprintf("cha=%c string=%s dec=%d hex=%x oct=%o neg=%d\n", 'A', "this is a test", 100, 100, 100, -100);

    myprintf("argc=%d\n", argc);

    for (int i = 0; i < argc; i++)
        myprintf("argv[%d] = %s\n", i, argv[i]);

    myprintf("\n");

    for (int i = 0; i < strlen(*env); i++)
        myprintf("env[%d] = %s\n", i, env[i]);

    testmyprintf();
}