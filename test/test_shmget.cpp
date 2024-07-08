//#include "cache_driver.h"
//#include "IO_smart_iterator.h"
#include <stdlib.h>
#include <sched.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <asm/mman.h>
//#include "pin_thread.h"
//#include "get_vert_count.hpp"
//#include "get_col_ranger.hpp"

//共享内存的实现
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <string.h>
#include <unistd.h>

//编译命令：g++ test_shmget.cpp -g -o test_shmget
//命令行：./test_shmget 1 1 2 ../../disk_management/manage_dest ../../disk_management/manage_dest com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 2048 2097152 1 16 3072441 1 1 2048
int main(int argc, char **argv) 
{
	std::cout<<"Format: /path/to/exe " 
		<<"#row_partitions #col_partitions thread_count "
		<<"/path/to/beg_pos_dir /path/to/csr_dir "
		<<"beg_header csr_header num_chunks "
		<<"chunk_sz (#bytes) concurr_IO_ctx "
		<<"max_continuous_useless_blk ring_vert_count num_buffs source "
		<<"cache_sz(#bytes)\n";//指定存放csr的cache的大小

	if(argc != 16)
	{
		fprintf(stdout, "Wrong input\n");
		exit(-1);
	}

    //Output input
	for(int i=0;i<argc;i++)
		std::cout<<argv[i]<<" ";
	std::cout<<"\n";

	const int row_par = atoi(argv[1]);
	const int col_par = atoi(argv[2]);
	const int NUM_THDS = atoi(argv[3]);
	const char *beg_dir = argv[4];
	const char *csr_dir = argv[5];
	const char *beg_header=argv[6];
	const char *csr_header=argv[7];
	const long num_chunks = atoi(argv[8]);
	const size_t chunk_sz = atoi(argv[9]);
	const long io_limit = atoi(argv[10]);
	const long MAX_USELESS = atoi(argv[11]);
	const long ring_vert_count = atoi(argv[12]);
	const long num_buffs = atoi(argv[13]);
	int root = (int) atol(argv[14]);
	size_t csrshm_sz = atoi(argv[15]);//单位B

	//实现共享内存，csr_shmid表示该共享内存的id	
	int csr_shmid = shmget((key_t)26, csrshm_sz, IPC_CREAT|0666);
	if(csr_shmid == -1){
		perror("csr_shmget: ");
		exit(-1);
	}else std::cout<<"shared memory id: "<<csr_shmid<<"\n";

	unsigned int *csr_shm_ptr;// 指向共享内存
	csr_shm_ptr = (unsigned int *)shmat(csr_shmid, NULL, 0);
	if(csr_shm_ptr == (void*)-1){
		perror("csr_shmat: ");
		exit(-1);
	}

	int vertex_per_cache = csrshm_sz/sizeof(unsigned int);
	//读入beg_pos，保证读入的点的每个边都放入cache，如果只能放一部分则直接舍弃
	char beg_name[256];
	sprintf(beg_name, "%s/row_0_col_0/%s.0_0_of_%dx%d.bin", 
					beg_dir, beg_header, row_par, col_par);
	
	printf("start reading %s\n",beg_name);
	//重用ring_vert_count参数用于测试
	int vertex_count = ring_vert_count;
	printf("total vertex: %d, vertex per cache: %d\n",vertex_count, vertex_per_cache);

	FILE *beg_file = fopen(beg_name, "rb");
	int cache_end = 0;
	long *beg_ptr;
	if(beg_file != NULL)
	{
		unsigned int i = 0;
		size_t ret = fread(beg_ptr, sizeof(long), vertex_count, beg_file);
		//assert(ret == vertex_count);
		for(unsigned int i=0; i<vertex_count; i++){
			if(beg_ptr[i] > vertex_per_cache && i>=2){
				cache_end = i-1; //有i-1个点，但id应为i-2
				printf("cache end vertex id: %d\n",cache_end-1);
				break;
			}else if (beg_ptr[i] > vertex_per_cache && i<2){
				printf("no vertex in cache\n");
				cache_end = -1;
				break;
			}
		}
		fclose(beg_file);
		//先不考虑cache中没有点的情况	
		printf("beg_pos : ");
	    for(int j = cache_end-1; j < cache_end + 9; j++)
			printf("beg_pos[%d]:%u ", j, beg_ptr[j]);
	    printf("\n");

	}
	else{
		fprintf(stdout,"Wrong open %s\n",beg_name);
		perror("open");
		exit(-1);
	}
	
    //读入第0_0块的CSR
	char name[256];
	sprintf(name, "%s/row_0_col_0/%s.0_0_of_%dx%d.bin", 
					csr_dir, csr_header, row_par, col_par);
	//sprintf(name, "%s/row_0_col_0/%s-%dx%d-col-ranger.bin", 
	//		beg_dir, beg_header, row_par, col_par);
					
	printf("start reading %s\n",name);
	FILE *file = fopen(name, "rb");
	if(file != NULL)
	{
		size_t ret = fread(csr_shm_ptr, sizeof(int), beg_ptr[cache_end], file);
		assert(ret == beg_ptr[cache_end]);
		fclose(file);	
		printf("Parition : ");
	    for(int j = 0; j < 10; j++)
			printf("%u ", csr_shm_ptr[j]);
	    printf("\n");

	}
	else{
		fprintf(stdout,"Wrong open %s\n",name);
		perror("open");
		exit(-1);
	}
	//此处再起一个进程检查共享内存的内容
	sleep(60);

	//删除共享内存
	if (shmctl(csr_shmid, IPC_RMID, 0) == -1){
		printf("shmctl(0x0) failed\n");
	 	return -1;
	}
}