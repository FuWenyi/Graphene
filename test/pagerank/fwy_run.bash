#twitter-2010
#64KB partition 1x1 iteration 20
echo "sudo ./aio_pagerank.bin 1 1 2 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 20 > pagerank_output_1x1.txt"
sudo ./aio_pagerank.bin 1 1 2 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 20 > pagerank_output_1x1.txt
echo "mv iostat_pagerank.log iostat_pagerank_1x1.log"
mv iostat_pagerank.log iostat_pagerank_1x1.log
#64KB partition 1x2 iteration 20
echo "sudo ./aio_pagerank.bin 1 2 4 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 20 > pagerank_output_1x2.txt"
sudo ./aio_pagerank.bin 1 2 4 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 20 > pagerank_output_1x2.txt
echo "mv iostat_pagerank.log iostat_pagerank_1x2.log"
mv iostat_pagerank.log iostat_pagerank_1x2.log
#64KB partition 2x2 iteration 20
echo "sudo ./aio_pagerank.bin 2 2 8 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 20 > pagerank_output_2x2.txt"
sudo ./aio_pagerank.bin 2 2 8 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 20 > pagerank_output_2x2.txt
echo "mv iostat_pagerank.log iostat_pagerank_2x2.log"
mv iostat_pagerank.log iostat_pagerank_2x2.log

sudo ./aio_pagerank.bin 2 3 12 /home/fuwenyi/ssd /home/fuwenyi/ssd twitter.txt-split_beg twitter.txt-split_csr 16384 65536 32 16 2048 1 20 > pagerank_output_2x3.txt
echo "mv iostat_pagerank.log iostat_pagerank_2x3.log"
mv iostat_pagerank.log iostat_pagerank_2x3.log