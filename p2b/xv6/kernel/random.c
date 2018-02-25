static unsigned long X = 1;
int getrandom(int M) {
    unsigned long a = 1103515245, c = 12345;
    X = a * X + c; 
    return ((unsigned int)(X / 65536) % 32768) % M + 1;
}
