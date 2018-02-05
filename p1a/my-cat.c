#include <stdio.h>
#define LINE_MAX 1024

int main(int argc, char * argv []) {
	if(argc != 2) return -1;
	char * name = argv[1];
	FILE * fp = fopen(name, "r");
	if (fp == NULL) {
		printf("cannot open file\n");
	}
	char line[LINE_MAX];
	while (fgets(line, LINE_MAX, fp) != NULL) {
		printf("%s", line);
	}
	fclose(fp);
	return 0;
}
