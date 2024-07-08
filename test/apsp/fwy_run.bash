#twitter-2010

#64KB partition 1x1
#sudo ./aio_bfs.bin 1 1 2 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 1
#sudo ./aio_bfs.bin 1 1 2 /home/fuwenyi/ssd /home/fuwenyi/ssd com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 16384 65536 32 16 2048 1 1
#64KB partition 1x2
#sudo ./aio_bfs.bin 1 2 4 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 1
#64KB partition 2x2
#sudo ./aio_bfs.bin 2 2 8 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 1

#!/bin/bash

# 1x1
for ((i=1; i<=10; i++))
do
  echo "sudo ./aio_apsp.bin 2 2 8 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 >> apsp_10_2x2.txt"
  echo "mv ./10_log/iostat_apsp.log ./10_log/iostat_apsp${i}.log"
  eval "sudo ./aio_apsp.bin 2 2 8 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 >> apsp_10_2x2.txt"
  mv ./10_log/iostat_apsp.log ./10_log/iostat_apsp${i}.log
done
