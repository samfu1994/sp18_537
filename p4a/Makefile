all :
	gcc mapreduce.c -pthread -o test

debug :
	gcc mapreduce.c -g -pthread -o test

wc:
	gcc -o test_wc wordcount.c mapreduce.c -Wall -Werror -pthread -O -DNUM_MAPPERS=1 -DNUM_REDUCERS=10

sort:
	gcc -o test_sort sort.c mapreduce.c -Wall -Werror -pthread -O -DNUM_MAPPERS=1 -DNUM_REDUCERS=1

clean:	
	rm test
