#!/bin/bash
<<COMMENT
rm request_analyse.txt
#固定ring buffer size=4G, GAP=16

#com-orkut
#512
./aio_bfs.bin 1 1 2 ../../../disk_management/manage_dest ../../../disk_management/manage_dest com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 8388608 512 1 16 2048 1 1 2097152
#1024
./aio_bfs.bin 1 1 2 ../../../disk_management/manage_dest ../../../disk_management/manage_dest com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 4194304 1024 1 16 2048 1 1 2097152
#4096
./aio_bfs.bin 1 1 2 ../../../disk_management/manage_dest ../../../disk_management/manage_dest com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 1048576 4096 1 16 2048 1 1 2097152
#16K
./aio_bfs.bin 1 1 2 ../../../disk_management/manage_dest ../../../disk_management/manage_dest com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 262144 16384 1 16 2048 1 1 2097152
#64K
./aio_bfs.bin 1 1 2 ../../../disk_management/manage_dest ../../../disk_management/manage_dest com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 65536 65536 1 16 2048 1 1 2097152
#128K
./aio_bfs.bin 1 1 2 ../../../disk_management/manage_dest ../../../disk_management/manage_dest com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 32768 131072 1 16 2048 1 1 2097152
#512K
./aio_bfs.bin 1 1 2 ../../../disk_management/manage_dest ../../../disk_management/manage_dest com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 8192 524288 1 16 2048 1 1 2097152
#1MB
./aio_bfs.bin 1 1 2 ../../../disk_management/manage_dest ../../../disk_management/manage_dest com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 4096 1048576 1 16 2048 1 1 2097152

#graph500-22
#512
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-22_dest/ ../../../disk_management/graph500-22_dest/ edge_id.txt-split_beg edge_id.txt-split_csr 8388608 512 1 16 2048 1 0 2097152
#1024
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-22_dest/ ../../../disk_management/graph500-22_dest/ edge_id.txt-split_beg edge_id.txt-split_csr 4194304 1024 1 16 2048 1 0 2097152
#4096
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-22_dest/ ../../../disk_management/graph500-22_dest/ edge_id.txt-split_beg edge_id.txt-split_csr 1048576 4096 1 16 2048 1 0 2097152
#16K
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-22_dest/ ../../../disk_management/graph500-22_dest/ edge_id.txt-split_beg edge_id.txt-split_csr 262144 16384 1 16 2048 1 0 2097152
#64K
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-22_dest/ ../../../disk_management/graph500-22_dest/ edge_id.txt-split_beg edge_id.txt-split_csr 65536 65536 1 16 2048 1 0 2097152
#128K
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-22_dest/ ../../../disk_management/graph500-22_dest/ edge_id.txt-split_beg edge_id.txt-split_csr 32768 131072 1 16 2048 1 0 2097152
#512K
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-22_dest/ ../../../disk_management/graph500-22_dest/ edge_id.txt-split_beg edge_id.txt-split_csr 8192 524288 1 16 2048 1 0 2097152
#1MB
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-22_dest/ ../../../disk_management/graph500-22_dest/ edge_id.txt-split_beg edge_id.txt-split_csr 4096 1048576 1 16 2048 1 0 2097152
#COMMENT

#graph500-25
#512
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-25_dest/ ../../../disk_management/graph500-25_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 8388608 512 1 16 2048 1 2 2097152
#1024
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-25_dest/ ../../../disk_management/graph500-25_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 4194304 1024 1 16 2048 1 2 2097152
#4096
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-25_dest/ ../../../disk_management/graph500-25_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 1048576 4096 1 16 2048 1 2 2097152
#16K
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-25_dest/ ../../../disk_management/graph500-25_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 262144 16384 1 16 2048 1 2 2097152
#64K
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-25_dest/ ../../../disk_management/graph500-25_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 65536 65536 1 16 2048 1 2 2097152
#128K
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-25_dest/ ../../../disk_management/graph500-25_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 32768 131072 1 16 2048 1 2 2097152
#512K
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-25_dest/ ../../../disk_management/graph500-25_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 8192 524288 1 16 2048 1 2 2097152
#1MB
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-25_dest/ ../../../disk_management/graph500-25_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 4096 1048576 1 16 2048 1 2 2097152

#<<COMMENT
#graph500-26
#512
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-26_dest/ ../../../disk_management/graph500-26_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 8388608 512 1 16 2048 1 0 2097152
#1024
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-26_dest/ ../../../disk_management/graph500-26_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 4194304 1024 1 16 2048 1 0 2097152
#4096
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-26_dest/ ../../../disk_management/graph500-26_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 1048576 4096 1 16 2048 1 0 2097152
#16K
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-26_dest/ ../../../disk_management/graph500-26_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 262144 16384 1 16 2048 1 0 2097152
#64K
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-26_dest/ ../../../disk_management/graph500-26_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 65536 65536 1 16 2048 1 0 2097152
#128K
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-26_dest/ ../../../disk_management/graph500-26_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 32768 131072 1 16 2048 1 0 2097152
#512K
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-26_dest/ ../../../disk_management/graph500-26_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 8192 524288 1 16 2048 1 0 2097152
#1MB
./aio_bfs.bin 1 1 2 ../../../disk_management/graph500-26_dest/ ../../../disk_management/graph500-26_dest/ edge_id.csv-split_beg edge_id.csv-split_csr 4096 1048576 1 16 2048 1 0 2097152
COMMENT

#twitter-2010
#512
./aio_bfs.bin 1 1 2 ../../../disk_management/twitter-2010_dest/ ../../../disk_management/twitter-2010_dest/ edge.csv-split_beg edge.csv-split_csr 8388608 512 1 16 2048 1 0 2097152
#1024
./aio_bfs.bin 1 1 2 ../../../disk_management/twitter-2010_dest/ ../../../disk_management/twitter-2010_dest/ edge.csv-split_beg edge.csv-split_csr 4194304 1024 1 16 2048 1 0 2097152
#4096
./aio_bfs.bin 1 1 2 ../../../disk_management/twitter-2010_dest/ ../../../disk_management/twitter-2010_dest/ edge.csv-split_beg edge.csv-split_csr 1048576 4096 1 16 2048 1 0 2097152
#16K
./aio_bfs.bin 1 1 2 ../../../disk_management/twitter-2010_dest/ ../../../disk_management/twitter-2010_dest/ edge.csv-split_beg edge.csv-split_csr 262144 16384 1 16 2048 1 0 2097152
#64K
./aio_bfs.bin 1 1 2 ../../../disk_management/twitter-2010_dest/ ../../../disk_management/twitter-2010_dest/ edge.csv-split_beg edge.csv-split_csr 65536 65536 1 16 2048 1 0 2097152
#128K
./aio_bfs.bin 1 1 2 ../../../disk_management/twitter-2010_dest/ ../../../disk_management/twitter-2010_dest/ edge.csv-split_beg edge.csv-split_csr 32768 131072 1 16 2048 1 0 2097152
#512K
./aio_bfs.bin 1 1 2 ../../../disk_management/twitter-2010_dest/ ../../../disk_management/twitter-2010_dest/ edge.csv-split_beg edge.csv-split_csr 8192 524288 1 16 2048 1 0 2097152
#1MB
./aio_bfs.bin 1 1 2 ../../../disk_management/twitter-2010_dest/ ../../../disk_management/twitter-2010_dest/ edge.csv-split_beg edge.csv-split_csr 4096 1048576 1 16 2048 1 0 2097152

