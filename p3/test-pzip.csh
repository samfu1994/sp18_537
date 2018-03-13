#! /bin/csh -f
set TEST_HOME = /afs/cs.wisc.edu/p/course/cs537-remzi/tests
set source_file = pzip.c
set binary_file = pzip
set bin_dir = /afs/cs.wisc.edu/p/course/cs537-remzi/tests/bin
set test_dir = /afs/cs.wisc.edu/p/course/cs537-remzi/tests/tests-pzip
${bin_dir}/serialized_runner.py `date +%s` -1 ${bin_dir}/generic-tester.py $argv[*] -s $source_file -b $binary_file -t $test_dir -f="-pthread" --timed
