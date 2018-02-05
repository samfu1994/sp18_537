#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define NAME_MAX_LENGTH 1024
#define MAX_LENGTH 1024
#define DIGIT 32

int g_count = 0;
char g_char = '+';
char buffer[DIGIT];

void my_unzip(char * line) {
	int count = 0;
	for(int i = 3; i >= 0; i--) {
		count *= 16;
		count += (int)(line[i]);
	}
	char c = line[4];
	for(int i = 0; i < count; i++) {
		printf("%c", c);
	}
	return;
}

int main(int argc, char * argv []) {
	if(argc == 1) {
		printf("my-unzip: file1 [file2 ...]\n");
		exit(1);
	}
	int nums = argc - 1;
	int t;

	char * line = (char*) malloc(MAX_LENGTH * sizeof(char));
	FILE * fp;

	for(int i = 0; i < nums; i++) {
		fp = fopen(argv[i + 1], "r");
		if (fp == NULL) {
			printf("my-unzip: cannot open file\n");
			exit(1);
		}
		while((t = fread(line, 5, 1, fp)) == 1) {
			my_unzip(line);
		}
		fclose(fp);
	}
	return 0;
}
