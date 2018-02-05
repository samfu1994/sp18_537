#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define NAME_MAX_LENGTH 1024
#define MAX_LENGTH 10240
void find(char * item, char * line) {
	if(strstr(line, item) != NULL) {
		printf("%s", line);
	}
	return;
}
int main(int argc, char * argv []) {
	if(argc == 1) {
		printf("my-grep: searchterm [file ...]\n");
		exit(1);
	}
	int nums = argc - 2;
	size_t current_length = 0;
	int t;
	char * line = (char*) malloc(MAX_LENGTH * sizeof(char));
	char * item = argv[1];
	FILE * fp;
	if(nums == 0) {
		while((t = getline(&line, &current_length, stdin)) != -1) {
			find(item, line);
		}
	}

	for(int i = 0; i < nums; i++) {
		fp = fopen(argv[i + 2], "r");
		if (fp == NULL) {
			printf("my-grep: cannot open file\n");
			exit(1);
		}
		while((t = getline(&line, &current_length, fp)) != -1) {
			find(item, line);
		}	
		fclose(fp);
	}
	return 0;
}
