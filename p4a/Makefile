# all: mapreduce.o thpool.o
# 	gcc mapreduce.o thpool.o -o test

thpool.o:	thpool.c
		gcc -c -pthread thpool.c

test: thpool.o mapreduce.c 
		gcc mapreduce.c thpool.o -pthread -o test

clean:
	rm *.o
	rm test
