#!/bin/bash
TEST_DIR=/u/c/s/cs537-1/ta/tests/4a-new
INPUT_DIR=/u/c/s/cs537-1/ta/tests/4a-new/inputs
OUTPUT_DIR=/u/c/s/cs537-1/ta/tests/4a-new/outputs

#flags
delete_flag=0

function check_mr_files {
	if [ ! -f mapreduce.c ]; then
	    echo "mapreduce.c file not found!"
	    exit 1
	fi

	if [ ! -f mapreduce.h ]; then
	    echo "mapreduce.h file not found!"
	    exit 1
	fi
}
function wordcount_p_run {
	echo "[Performance Test - Wordcounting ${3} file(s) with ${1} mapper(s) and ${2} reducer(s) and default partitioner]"
	echo "(${5})"
	echo "  Running \$./wordcount_performance ${4}"

	if ! gcc -o wordcount_performance wordcount.c mapreduce.c -g -Wall -Werror -pthread -O -lm -DNUM_MAPPERS=${1} -DNUM_REDUCERS=${2} -DFILE_OUTPUT_SUFFIX="\"performance\""; then
		echo "[Test failed]"
    	echo "  Failed to build wordcount_performance"
    	echo
    	exit 1
	fi

	if ! ./wordcount_performance ${4}; then
		echo "[Test failed]"
		echo "  Error occured while running ./wordcount_performance"
    	echo
    	exit 1
	fi

	for i in `seq 0 $(expr $2 - 1)`;
        do
			if ! cmp --silent ${OUTPUT_DIR}/wordcount_performance\($i\).out wordcount_performance\($i\).out; then
				echo "[Test${test_cnt} failed]"
				echo "  The output is different from the expected output."
				echo "    - expected output path: ${OUTPUT_DIR}/wordcount_performance\($i\).out"
				echo "    - actual output path: wordcount_performance\($i\).out"
    			echo

		    	exit 1
			fi
        done
    echo "[Test Done]"
    echo
	#delete executable on success
	if [ ${delete_flag} -eq 1 ]; then
		rm -f ./wordcount_m${1}r${2}
		for i in `seq 0 $(expr $2 - 1)`;
	        do
				rm -f wordcount_performance\($i\).out
	        done
	fi
}

while [[ $# -gt 0 ]]
do
key="$1"
case $key in
    -d)
	delete_flag=1
	shift
    ;;
esac
done
# actual script here
cp -f ${TEST_DIR}/wordcount.c .
check_mr_files

# *** wordcount ***
# wordcount_run num_map num_reduce num_file inputargs filedesc
wordcount_p_run 4 4 8 "${INPUT_DIR}/file100m.in ${INPUT_DIR}/file100m.in ${INPUT_DIR}/file100m.in ${INPUT_DIR}/file100m.in ${INPUT_DIR}/file10m.in ${INPUT_DIR}/file10m.in ${INPUT_DIR}/file5m.in ${INPUT_DIR}/file1m.in" \
"100mb file *4 , 10mb file *2 , 5mb file, 1mb file"

if [ ${delete_flag} -eq 1 ]; then
	echo "Delete *.out files"
	rm -f wordcount_performance*
fi

exit 0
