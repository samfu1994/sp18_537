# all: mapreduce.o thpool.o
# 	gcc mapreduce.o thpool.o -o test

thpool.o:	thpool.c
		gcc -Wall -Werror -c -pthread thpool.c

test: thpool.o mapreduce.c 
		gcc mapreduce.c thpool.o -Wall -Werror -pthread -o test

debug: thpool.o mapreduce.c 
		gcc mapreduce.c thpool.o -O0 -Wall -Werror -pthread -o test


clean:
	rm *.o
	rm test
