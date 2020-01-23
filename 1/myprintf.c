#include <stdio.h>

typedef unsigned int u32;

char *ctable = "0123456789ABCDEF";
int BASE_h = 16;
int BASE_d = 10;
int BASE_o = 8;

int rpu(u32 x) {
    char c;
    if (x) {
        c = ctable[x % BASE_d];
        rpu(x / BASE_d);
        putchar(c);
    }
}

int rph(u32 x) {
    char c;
    if (x) {
        c = ctable[x % BASE_h];
        rph(x / BASE_h);
        putchar(c);
    }
}

int rpo(u32 x) {
    char c;
    if (x) {
        c = ctable[x % BASE_o];
        rpo(x / BASE_o);
        putchar(c);
    }
}

int printc(char x) { putchar(x); }

int prints(char *x) {
    for(int i = 0; x[i] != '\0'; i++)
        printc(x[i]);
}

int printu(u32 x) {
    (x == 0) ? putchar('0') : rpu(x);
    putchar(' ');
}

int printd(int x) {
    if (x < 0)
        putchar('-');
    rpu(x);    
}

int printo(u32 x) {
    putchar('0');
    (x == 0) ? putchar('0') : rpo(x);
}

int printx(u32 x) {
    putchar('0');
    putchar('x');
    
    (x == 0) ? putchar('0') : rph(x);
}

int myprintf(char *fmt, ...) {
    char x;
    int *ip = (int *) &fmt;
    for (int i = 0; fmt[i] != '\0'; i++) {        
        if (fmt[i] != '%') {
            putchar(fmt[i]);
            continue;
        }

        ip++;

        switch (fmt[++i]) {
            case 'c':
                printc((char) *ip);
                break;
            case 's':
                prints((char *) *ip);
                break;
            case 'u':
                printu((u32) *ip);
                break;
            case 'd':
                printd((int) *ip);
                break;
            case 'o':
                printo((u32) *ip);
                break;
            case 'x':
                printx((u32) *ip);
                break;
            default:
                ip--;
                putchar('%');
                putchar(fmt[i]);
        }
    }
}

int main(int argc, char *argv[], char *env[]) {
    myprintf("argc:\t\t%d\n", argc);
    for(int i = 0; i < argc; i++)
        myprintf("argv[%d]: \t%s\n", i, argv[i]);
}