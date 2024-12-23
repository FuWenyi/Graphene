#twitter-2010

#64KB partition 1x1
#sudo ./aio_bfs.bin 1 1 2 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 1
#                                                                                                                         chunk_num chunk_size
#sudo ./aio_bfs.bin 1 1 2 /home/fuwenyi/ssd /home/fuwenyi/ssd com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 16384 65536 32 16 2048 1 1
#64KB partition 1x2
#sudo ./aio_bfs.bin 1 2 4 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 1
#64KB partition 2x2
#sudo ./aio_bfs.bin 2 2 8 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 1
# numactl --physcpubind=+6-11,18-23

#!/bin/bash

# read random node 
IFS=',' read -r -a values < <(sed 's/ //g' random_20.txt)

echo "Root Node ID: "

for value in "${values[@]}"; do
  echo "$value"
done

# origin command
command="sudo ./aio_bfs.bin 3 4 24 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 id >> random_20_3x4.log"

# every command
for value in "${values[@]}"
do
  echo "${command//id/$value}"
  new_log_name="iostat_bfs_${value}.log"
  echo "mv ./random_20_log/iostat_bfs.log ./random_20_log/$new_log_name"
  eval "${command//id/$value}"
  mv ./random_20_log/iostat_bfs.log ./random_20_log/$new_log_name
done
