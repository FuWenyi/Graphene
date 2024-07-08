#twitter-2010
#64KB partition 1x1 iteration 20
for ((i=1; i<=10; i++))
do
  echo "sudo ./aio_spmv.bin 3 4 24 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 >> spmv_output_3x4.txt"
  sudo ./aio_spmv.bin 3 4 24 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 >> spmv_output_3x4.txt
  echo "mv ./log/iostat_spmv.log ./log/iostat_spmv_3x4_${i}.log"
  mv ./log/iostat_spmv.log ./log/iostat_spmv_3x4_${i}.log
done