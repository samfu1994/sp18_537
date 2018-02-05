#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define NAME_MAX_LENGTH 1024
#define MAX_LENGTH 1024
int g_count = 0;
char g_char = '+';

void my_zip(char * line, int current_length) {
	for(int i = 0; i < current_length; i++) {		
		if(line[i] != g_char) {
			if(g_count != 0){
				fwrite((const void*) &g_count,sizeof(int), 1, stdout);
				fwrite((const void*) &g_char, sizeof(char), 1, stdout);
			}
			g_char = line[i];
			g_count = 1;
		}
		else {
			g_count++;
		}
	}
}

int main(int argc, char * argv []) {
	if(argc == 1) {
		printf("my-zip: file1 [file2 ...]\n");
		exit(1);
	}
	int nums = argc - 1;
	size_t current_length = 0;
	int t;

	char * line = (char*) malloc(MAX_LENGTH * sizeof(char));
	FILE * fp;

	for(int i = 0; i < nums; i++) {
		fp = fopen(argv[i + 1], "r");
		if (fp == NULL) {
			printf("my-zip: cannot open file\n");
			exit(1);
		}
		while((t = getline(&line, &current_length, fp)) != -1) {
			my_zip(line, t);
		}	
		fclose(fp);
	}
	if(g_count != 0){
		fwrite((const void*) &g_count,sizeof(int),1,stdout);
		fwrite((const void*) &g_char, sizeof(char), 1, stdout);
	}
	return 0;
}
