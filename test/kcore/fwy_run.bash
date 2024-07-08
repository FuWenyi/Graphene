#twitter-2010

#64KB partition 1x1
#sudo ./aio_bfs.bin 1 1 2 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 1
#sudo ./aio_bfs.bin 1 1 2 /home/fuwenyi/ssd /home/fuwenyi/ssd com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 16384 65536 32 16 2048 1 1
#64KB partition 1x2
#sudo ./aio_bfs.bin 1 2 4 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 1
#64KB partition 2x2
#sudo ./aio_bfs.bin 2 2 8 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 1

#!/bin/bash

# read random node 
IFS=',' read -r -a values < <(sed 's/ //g' random_20.txt)

echo "K num: "

for value in "${values[@]}"; do
  echo "$value"
done

# origin command
command="sudo ./aio_kcore.bin 2 2 8 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 knum >> k_2x2.log"

# every command
for value in "${values[@]}"
do
  echo "${command//knum/$value}"
  new_log_name="iostat_kcore_${value}.log"
  echo "mv ./log/iostat_kcore.log ./log/$new_log_name"
  eval "${command//knum/$value}"
  mv ./log/iostat_kcore.log ./log/$new_log_name
done
