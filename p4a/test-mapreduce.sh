#!/bin/bash
TEST_DIR=/u/c/s/cs537-1/ta/tests/4a-new
INPUT_DIR=/u/c/s/cs537-1/ta/tests/4a-new/inputs
OUTPUT_DIR=/u/c/s/cs537-1/ta/tests/4a-new/outputs

#flags
continue_flag=0
delete_flag=0

#results
test_cnt=0
test_passed=0

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

function sort_run {
	test_cnt=$((test_cnt+1))
	echo "[Test${test_cnt} - Sorting ${3} file(s) with ${1} mapper(s), ${2} reducer(s) and default partitioner]"
	echo "  Running \$./sort_m${1}r${2} ${4}"

	if ! gcc -o sort_m${1}r${2} sort.c mapreduce.c -g -Wall -Werror -pthread -O -lm -DNUM_MAPPERS=${1} -DNUM_REDUCERS=${2} -DFILE_OUTPUT_SUFFIX="\"f${3}m${1}r${2}_t${test_cnt}\""; then
		echo "[Test${test_cnt} failed]"
    	echo "  Failed to build sort_m${1}r${2}"
    	echo
    	if [ ${continue_flag} -eq 1 ]; then
    		return 1	
    	fi
    	exit 1
	fi

	if ! ./sort_m${1}r${2} ${4}; then
		echo "[Test${test_cnt} failed]"
		echo "  Error occured while running ./sort_m${1}r${2} ${4}"
    	echo
    	#delete executable on failure
    	if [ ${continue_flag} -eq 1 ]; then
    		return 1	
    	fi
    	exit 1
	fi

	for i in `seq 0 $(expr $2 - 1)`;
        do
			if ! cmp --silent ${OUTPUT_DIR}/sort_f${3}m${1}r${2}_t${test_cnt}\($i\).out sort_f${3}m${1}r${2}_t${test_cnt}\($i\).out; then
				echo "[Test${test_cnt} failed]"
				echo "  The output is different from the expected output."
				echo "    - expected output path: ${OUTPUT_DIR}/sort_f${3}m${1}r${2}_t${test_cnt}\($i\).out"
				echo "    - actual output path: sort_f${3}m${1}r${2}_t${test_cnt}\($i\).out"
    			echo
		    	if [ ${continue_flag} -eq 1 ]; then
		    		return 1	
		    	fi
		    	exit 1
			fi
        done
    echo "[Test${test_cnt} Success]"
    echo
	test_passed=$((test_passed+1))

	#delete executable on success
	if [ ${delete_flag} -eq 1 ]; then
		rm -f ./sort_m${1}r${2}
		for i in `seq 0 $(expr $2 - 1)`;
	        do
				rm -f sort_f${3}m${1}r${2}_t${test_cnt}\($i\).out
	        done
	fi
}

function wordcount_run {
	test_cnt=$((test_cnt+1))
	echo "[Test${test_cnt} - Wordcounting ${3} file(s) with ${1} mapper(s) and ${2} reducer(s) and default partitioner]"
	echo "(${5})"
	echo "  Running \$./wordcount_m${1}r${2} ${4}"

	if ! gcc -o wordcount_m${1}r${2} wordcount.c mapreduce.c -g -Wall -Werror -pthread -O -lm -DNUM_MAPPERS=${1} -DNUM_REDUCERS=${2} -DFILE_OUTPUT_SUFFIX="\"f${3}m${1}r${2}_t${test_cnt}\""; then
		echo "[Test${test_cnt} failed]"
    	echo "  Failed to build wordcount_m${1}r${2}"
    	echo
    	if [ ${continue_flag} -eq 1 ]; then
    		return 1	
    	fi
    	exit 1
	fi

	if ! ./wordcount_m${1}r${2} ${4}; then
		echo "[Test${test_cnt} failed]"
		echo "  Error occured while running ./wordcount_m${1}r${2} ${4}"
    	echo
    	if [ ${continue_flag} -eq 1 ]; then
    		return 1	
    	fi
    	exit 1
	fi

	for i in `seq 0 $(expr $2 - 1)`;
        do
			if ! cmp --silent ${OUTPUT_DIR}/wordcount_f${3}m${1}r${2}_t${test_cnt}\($i\).out wordcount_f${3}m${1}r${2}_t${test_cnt}\($i\).out; then
				echo "[Test${test_cnt} failed]"
				echo "  The output is different from the expected output."
				echo "    - expected output path: ${OUTPUT_DIR}/wordcount_f${3}m${1}r${2}_t${test_cnt}\($i\).out"
				echo "    - actual output path: wordcount_f${3}m${1}r${2}_t${test_cnt}\($i\).out"
    			echo

		    	if [ ${continue_flag} -eq 1 ]; then
		    		return 1	
		    	fi
		    	exit 1
			fi
        done
    echo "[Test${test_cnt} Success]"
    echo
	test_passed=$((test_passed+1))
	#delete executable on success
	if [ ${delete_flag} -eq 1 ]; then
		rm -f ./wordcount_m${1}r${2}
		for i in `seq 0 $(expr $2 - 1)`;
	        do
				rm -f wordcount_f${3}m${1}r${2}_t${test_cnt}\($i\).out
	        done
	fi
}

function wordcount_cp_run {
	test_cnt=$((test_cnt+1))
	echo "[Test${test_cnt} - Wordcounting ${3} file(s) with ${1} mapper(s) and ${2} reducer(s) and custom partitioner]"
	echo "(${5})"
	echo "  Running \$./wordcount_cp_m${1}r${2} ${4}"

	if ! gcc -o wordcount_cp_m${1}r${2} wordcount_cp.c mapreduce.c -g -Wall -Werror -pthread -O -lm -DNUM_MAPPERS=${1} -DNUM_REDUCERS=${2} -DFILE_OUTPUT_SUFFIX="\"f${3}m${1}r${2}_t${test_cnt}\""; then
		echo "[Test${test_cnt} failed]"
    	echo "  Failed to build wordcount_cp_m${1}r${2}"
    	echo
    	if [ ${continue_flag} -eq 1 ]; then
    		return 1	
    	fi
    	exit 1
	fi

	if ! ./wordcount_cp_m${1}r${2} ${4}; then
		echo "[Test${test_cnt} failed]"
		echo "  Error occured while running ./wordcount_cp_m${1}r${2} ${4}"
    	echo
    	if [ ${continue_flag} -eq 1 ]; then
    		return 1	
    	fi
    	exit 1
	fi

	for i in `seq 0 $(expr $2 - 1)`;
        do
			if ! cmp --silent ${OUTPUT_DIR}/wordcount_cp_f${3}m${1}r${2}_t${test_cnt}\($i\).out wordcount_cp_f${3}m${1}r${2}_t${test_cnt}\($i\).out; then
				echo "[Test${test_cnt} failed]"
				echo "  The output is different from the expected output."
				echo "    - expected output path: ${OUTPUT_DIR}/wordcount_cp_f${3}m${1}r${2}_t${test_cnt}\($i\).out"
				echo "    - actual output path: wordcount_cp_f${3}m${1}r${2}_t${test_cnt}\($i\).out"
    			echo

		    	if [ ${continue_flag} -eq 1 ]; then
		    		return 1	
		    	fi
		    	exit 1
			fi
        done
    echo "[Test${test_cnt} Success]"
    echo
	test_passed=$((test_passed+1))
	#delete executable on success
	if [ ${delete_flag} -eq 1 ]; then
		rm -f ./wordcount_cp_m${1}r${2}
		for i in `seq 0 $(expr $2 - 1)`;
	        do
				rm -f wordcount_cp_f${3}m${1}r${2}_t${test_cnt}\($i\).out
	        done
	fi
}

# mode select

while [[ $# -gt 0 ]]
do
key="$1"
case $key in
    -c)
	continue_flag=1
	shift
    ;;
    -d)
	delete_flag=1
	shift
    ;;
esac
done

# actual script here
cp -f ${TEST_DIR}/sort.c .
cp -f ${TEST_DIR}/wordcount.c .
check_mr_files

# sort_run num_map num_reduce num_file inputargs
sort_run 1 1 1 "${INPUT_DIR}/wordlist.in"
sort_run 4 1 1 "${INPUT_DIR}/wordlist.in"
sort_run 1 4 1 "${INPUT_DIR}/wordlist.in"
sort_run 4 4 1 "${INPUT_DIR}/wordlist.in"

sort_run 1 1 4 "${INPUT_DIR}/wordlist.in ${INPUT_DIR}/wordlist.in ${INPUT_DIR}/wordlist.in ${INPUT_DIR}/wordlist.in"
sort_run 4 4 4 "${INPUT_DIR}/wordlist.in ${INPUT_DIR}/wordlist.in ${INPUT_DIR}/wordlist.in ${INPUT_DIR}/wordlist.in"

# wordcount_run num_map num_reduce num_file inputargs filedesc
wordcount_run 1 1 1 "${INPUT_DIR}/file1m.in" \
"1mb file"
wordcount_run 1 1 1 "${INPUT_DIR}/file100m.in" \
"100mb file"

wordcount_run 1 4 1 "${INPUT_DIR}/file1m.in" \
"1mb file"
wordcount_run 1 20 1 "${INPUT_DIR}/file1m.in" \
"1mb file"

wordcount_run 1 1 20 "${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in" \
"1mb file * 20"
wordcount_run 4 1 20 "${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in" \
"1mb file * 20"
wordcount_run 1 4 20 "${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in" \
"1mb file * 20"
wordcount_run 20 1 20 "${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in" \
"1mb file * 20"
wordcount_run 1 20 20 "${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in" \
"1mb file * 20"

wordcount_run 5 1 5 "${INPUT_DIR}/file100m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in ${INPUT_DIR}/file1m.in" \
"100mb file *1 , 1mb file * 4"
#wordcount_run 5 1 5 "${INPUT_DIR}/file100m.in ${INPUT_DIR}/file100m.in ${INPUT_DIR}/file100m.in ${INPUT_DIR}/file100m.in ${INPUT_DIR}/file1m.in" \
#"100mb file *4 , 1mb file * 1"

# custom partition

echo "Test Result (${test_passed}/${test_cnt})"

if [ ${delete_flag} -eq 1 ]; then
	echo "Delete *.out files"
	rm -f *.out
fi

exit 0
