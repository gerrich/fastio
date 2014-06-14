fastio
======
libs and tools for fast string/stream processing tasks

pp - run streaming jobs in parallel on multiprocessor machine

USAGE: seq 100000 | pp 20 /usr/bin/perl -lane 'print "$$ $. $_";' > /tmp/parallel_processed_file.txt

TODO:
one can run the same on cluster using ssh:
seq 100000 | pp 20 ssh <get_nth_host>  perl -lane 'print "$$ $. $_";' > /tmp/remote_processed_file.txt

TODO:
tool_like xargs with stdout/stderr gather
ls -1 | xargs_pp -I{} -P24 grep -oE "\w\w*" > /tmp/word_matches.txt

