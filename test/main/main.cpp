#include "cache_driver.h"
#include "IO_smart_iterator.h"
#include <stdlib.h>
#include <sched.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <asm/mman.h>
#include "pin_thread.h"
#include "get_vert_count.hpp"
#include "get_col_ranger.hpp"
#include "bfs_iter.hpp"
#include "util.h"

//BFS执行命令：
//sudo ./aio_main.bin 1 1 2 /home/mi/disk/dataset/manage_dest/ /home/mi/disk/dataset/manage_dest/ com-orkut.ungraph.txt-split_beg com-orkut.ungraph.txt-split_csr 1024 2097152 1 16 2048 1 0 /home/mi/disk/graphene/test_dataset/1.txt 3

int main(int argc, char **argv) 
{
	std::cout<<"Format: /path/to/exe " 
		<<"#row_partitions #col_partitions thread_count "
		<<"/path/to/beg_pos_dir /path/to/csr_dir "
		<<"beg_header csr_header num_chunks "
		<<"chunk_sz(#bytes) concurr_IO_ctx "
		<<"max_continuous_useless_blk ring_vert_count num_buffs"
		<<"job(BFS:0)"//指定图算法
		<<"root_file root_num\n"; //指定根节点

	if(argc != 17)
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
	const index_t num_chunks = atoi(argv[8]);
	const size_t chunk_sz = atoi(argv[9]);
	const index_t io_limit = atoi(argv[10]);
	const index_t MAX_USELESS = atoi(argv[11]);
	const index_t ring_vert_count = atoi(argv[12]);
	const index_t num_buffs = atoi(argv[13]);
	const int job = atoi(argv[14]);
	const char *root_file = argv[15];
	const int root_num = atoi(argv[16]);

	//size_t csrshm_sz = atoi(argv[15]);

	//vertex_t **csr = new vertex_t*[num_rows*num_cols];

	assert(NUM_THDS==(row_par*col_par*2));
	sa_t *sa = NULL;
	index_t *comm = new index_t[NUM_THDS];
	vertex_t **front_queue_ptr;
	index_t *front_count_ptr;
	vertex_t *col_ranger_ptr;
	
	const index_t vert_count=get_vert_count
		(comm, beg_dir,beg_header,row_par,col_par);
	get_col_ranger(col_ranger_ptr, front_queue_ptr,
			front_count_ptr, beg_dir, beg_header,
			row_par, col_par);
	
	/*sa=(sa_t *)mmap(NULL,sizeof(sa_t)*vert_count,
			PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS 
			| MAP_HUGETLB | MAP_HUGE_2MB, 0, 0);
	//| MAP_HUGETLB , 0, 0);
	if(sa==MAP_FAILED)
	{	
		perror("mmap");
		exit(-1);
	}*/

	const index_t vert_per_blk = chunk_sz / sizeof(vertex_t);
	if(chunk_sz&(sizeof(vertex_t) - 1))
	{
		std::cout<<"Page size wrong\n";
		exit(-1);
	}
	sa_t *sa_dummy=NULL;
	
	//记录每次request
	remove("io_comp_time.txt");

	int *semaphore_acq = new int[1];
	int *semaphore_flag = new int[1];

	//	gpu_semaphore[0]=1;
	//omp_lock_t gpu_semaphore;
	//omp_init_lock(&gpu_semaphore);
	//0 1 2 3 4 5 6 7 8 9 10 11 12 13 28 29 30 31 32 33 34 35 36 37 38 39 40 41
	//14 15 16 17 18 19 20 21 22 23 24 25 26 27 42 43 44 45 46 47 48 49 50 51 52 53 54 55
	//int core_id[8]={0, 2, 4, 6, 14, 16, 18, 20};
	//int core_id[16]={0, 2, 4, 6, 8, 10, 12, 28, 14, 16, 18, 20, 22, 24, 26, 42};
	//int core_id[16]={0, 2, 4, 6, 8, 10, 12, 28, 30, 32, 34, 36, 38, 40, 1, 3};

	//0 1 2 3 4 5 12 13 14 15 16 17
	//6 7 8 9 10 11 18 19 20 21 22 23
//	int socket_one[12]={0, 6, 1, 7, 2, 8, 3, 9, 4, 10, 5, 11};
//	int socket_two[12]={12, 18, 13, 19, 14, 20, 15, 21, 16, 22, 17, 23};
	
	int socket_one[12]={0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17};
	int socket_two[12]={6, 7, 8, 9, 10, 11, 18, 19, 20, 21, 22, 23};

	// 存放total time的数组
	double *total_times = new double[root_num];
	double *total_comp_time = new double[root_num];
	double *total_io_time = new double[root_num];
	// double *total_convert_time = new double[root_num];
	// double *total_wait_io_time  = new double[root_num];
	// // double *total_comp_cache_time = new double[root_num]; 
	// double *total_io_submit_time = new double[root_num];
	// double *total_poll_time = new double[root_num];
	// double *total_wait_comp_time = new double[root_num];
	// double *total_wait_aio_time = new double[root_num];
	// double *total_comp_chunk_bfs_time = new double[root_num];
	// // double *total_translator_map_time1 = new double[root_num];
	// // double *total_translator_map_time2 = new double[root_num];
	// // double *total_comp_find_in_map_time = new double[root_num];
	// // double *total_comp_get_addr_map_time = new double[root_num];
	// // double *total_get_cache_miss_time = new double[root_num];
	// // double *total_put_cache_miss_time = new double[root_num];
	// long *total_submitted_ctx_count = new long[root_num];
	// long *total_submitted_full_ctx_count = new long[root_num];
	// long *total_reqt_blk_count = new long[root_num];

if(job == 0){
		//将根节点数组读出	
		sa=(sa_t *)mmap(NULL,sizeof(sa_t)*vert_count,
			PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS 
			| MAP_HUGETLB | MAP_HUGE_2MB, 0, 0);
		if(sa==MAP_FAILED){	
			perror("mmap");
			exit(-1);
		}
		vertex_t *root_l = new vertex_t[root_num];
		FILE *file = fopen(root_file, "rb");
		if(file == NULL) {
		printf("root文件读取无效: %s\n", root_file);
		}
		char str[100];
		for(int i=0;i<root_num;i++){
			fgets(str, 100, file);
			sscanf(str, "%d", &root_l[i]);
		}
		//fread(root_l, sizeof(vertex_t), root_num, file);
		fclose(file);

		//由于每次BFS会改变comm的值，为防止每次都读一遍beg文件，使用一个副本进行BFS
		index_t *comm_sgl = new index_t[NUM_THDS];

		// 开始计时
		clock_t startTime,endTime;
		startTime = clock();

		for(int i=0; i<root_num;i++){
			memset(sa, INFTY, sizeof(sa_t)*vert_count);
			
			memcpy(comm_sgl, comm, sizeof(comm));	
			// vertex_t root = root_l[i];
			//printf("begin BFS: %d\n",root);
			bfs_result result = run_bfs(row_par, col_par, NUM_THDS, beg_dir, csr_dir,
								beg_header, csr_header, num_chunks,
								chunk_sz, io_limit,MAX_USELESS, ring_vert_count,
								num_buffs, root_l[i], sa, comm_sgl, front_queue_ptr, front_count_ptr, 
								col_ranger_ptr, vert_count, vert_per_blk, sa_dummy);

			total_times[i] = result.total_time;
			total_comp_time[i] = result.total_comp_time;
			total_io_time[i] = result.total_io_time;
			// total_convert_time[i] = result.total_convert_time;
			// total_wait_io_time[i] = result.total_wait_io_time;
			// // total_comp_cache_time[i] = result.total_comp_cache_time;
			// total_io_submit_time[i] = result.total_io_submit_time;
			// total_poll_time[i] = result.total_poll_time;
			// total_wait_comp_time[i] = result.total_wait_comp_time;
			// total_wait_aio_time[i] = result.total_wait_aio_time;
			// total_comp_chunk_bfs_time[i] = result.total_comp_chunk_bfs_time;
			// total_submitted_ctx_count[i] = result.total_submitted_ctx_count;
			// total_submitted_full_ctx_count[i] = result.total_submitted_full_ctx_count;
			// total_reqt_blk_count[i] = result.total_reqt_blk_count;

			if(/*i != 0 &&*/ i % 100 == 0) {
				printf("当前遍历到第%d个点\n", i);
			}
		}
		endTime = clock();

		delete[] comm_sgl;
		delete[] root_l;
		
		munmap(sa,sizeof(sa_t)*vert_count);

		// 将结果写入文件
		FILE* fp1 = fopen("total_times.txt", "a");
		if(fp1 == NULL) {
			printf("文件读取无效.\n");
		}
		FILE* fp3 = fopen("total_comp_time.txt", "a");
		if(fp3== NULL) {
			printf("文件读取无效.\n");
		}
		FILE* fp4 = fopen("total_io_time.txt", "a");
		if(fp4== NULL) {
			printf("文件读取无效.\n");
		}
		// FILE* fp5 = fopen("total_convert_time.txt", "a");
		// if(fp5== NULL) {
		// 	printf("文件读取无效.\n");
		// }
		// FILE* fp6 = fopen("total_wait_io_time.txt", "a");
		// if(fp6== NULL) {
		// 	printf("文件读取无效.\n");
		// }
		// FILE* fp8 = fopen("total_io_submit_time.txt", "a");
		// if(fp8== NULL) {
		// 	printf("文件读取无效.\n");
		// }
		// FILE* fp9 = fopen("total_poll_time.txt", "a");
		// if(fp9== NULL) {
		// 	printf("文件读取无效.\n");
		// }
		// FILE* fp10 = fopen("total_wait_comp_time.txt", "a");
		// if(fp10== NULL) {
		// 	printf("文件读取无效.\n");
		// }
		// FILE* fp11 = fopen("total_wait_aio_time.txt", "a");
		// if(fp11== NULL) {
		// 	printf("文件读取无效.\n");
		// }
		// FILE* fp12 = fopen("total_comp_chunk_bfs_time.txt", "a");
		// if(fp12== NULL) {
		// 	printf("文件读取无效.\n");
		// }
		// FILE* fp19 = fopen("total_submitted_ctx_count.txt", "a");
		// if(fp19== NULL) {
		// 	printf("文件读取无效.\n");
		// }
		// FILE* fp20 = fopen("total_submitted_full_ctx_count.txt", "a");
		// if(fp20== NULL) {
		// 	printf("文件读取无效.\n");
		// }
		// FILE* fp21 = fopen("total_reqt_blk_count.txt", "a");
		// if(fp21== NULL) {
		// 	printf("文件读取无效.\n");
		// }


		fprintf(fp1, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);
		fprintf(fp3, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);
		fprintf(fp4, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);
		// fprintf(fp5, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);
		// fprintf(fp6, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);
		// fprintf(fp8, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);
		// fprintf(fp9, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);
		// fprintf(fp10, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);
		// fprintf(fp11, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);
		// fprintf(fp12, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);
		// fprintf(fp19, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);
		// fprintf(fp20, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);
		// fprintf(fp21, "chunk_sz为%ld, 运行总时间为%fs\n", chunk_sz, (double)(endTime - startTime) / CLOCKS_PER_SEC);

		for(int i = 0; i < root_num - 1; i++) {
			fprintf(fp1, "%f ",total_times[i]);
			// fprintf(fp2, "%f ",cache_miss[i]);
			fprintf(fp3, "%f ",total_comp_time[i]);
			fprintf(fp4, "%f ",total_io_time[i]);
			// fprintf(fp5, "%f ",total_convert_time[i]);
			// fprintf(fp6, "%f ",total_wait_io_time[i]);
			// fprintf(fp8, "%f ",total_io_submit_time[i]);
			// fprintf(fp9, "%f ",total_poll_time[i]);
			// fprintf(fp10, "%f ",total_wait_comp_time[i]);
			// fprintf(fp11, "%f ",total_wait_aio_time[i]);
			// fprintf(fp12, "%f ",total_comp_chunk_bfs_time[i]);
			// fprintf(fp19, "%ld ",total_submitted_ctx_count[i]);
			// fprintf(fp20, "%ld ",total_submitted_full_ctx_count[i]);
			// fprintf(fp21, "%ld ",total_reqt_blk_count[i]);
		}
		fprintf(fp1, "%f\n",total_times[root_num - 1]);
		// fprintf(fp2, "%f\n",cache_miss[root_num - 1]);
		fprintf(fp3, "%f\n",total_comp_time[root_num - 1]);
		fprintf(fp4, "%f\n",total_io_time[root_num - 1]);
		// fprintf(fp5, "%f\n",total_convert_time[root_num - 1]);
		// fprintf(fp6, "%f\n",total_wait_io_time[root_num - 1]);
		// // fprintf(fp7, "%f\n",total_comp_cache_time[root_num - 1]);
		// fprintf(fp8, "%f\n",total_io_submit_time[root_num - 1]);
		// fprintf(fp9, "%f\n",total_poll_time[root_num - 1]);
		// fprintf(fp10, "%f\n",total_wait_comp_time[root_num - 1]);
		// fprintf(fp11, "%f\n",total_wait_aio_time[root_num - 1]);
		// fprintf(fp12, "%f\n",total_comp_chunk_bfs_time[root_num - 1]);
		// fprintf(fp13, "%f\n",total_translator_map_time1[root_num - 1]);
		// fprintf(fp14, "%f\n",total_translator_map_time2[root_num - 1]);
		// fprintf(fp15, "%f\n",total_comp_find_in_map_time[root_num - 1]);
		// fprintf(fp16, "%f\n",total_comp_get_addr_map_time[root_num - 1]);
		// fprintf(fp17, "%f\n",total_get_cache_miss_time[root_num - 1]);
		// fprintf(fp18, "%f\n",total_put_cache_miss_time[root_num - 1]);
		// fprintf(fp19, "%ld\n",total_submitted_ctx_count[root_num - 1]);
		// fprintf(fp20, "%ld\n",total_submitted_full_ctx_count[root_num - 1]);
		// fprintf(fp21, "%ld\n",total_reqt_blk_count[root_num - 1]);
		fclose(fp1);
		// fclose(fp2);
		fclose(fp3);
		fclose(fp4);
		// fclose(fp5);
		// fclose(fp6);
		// // fclose(fp7);
		// fclose(fp8);
		// fclose(fp9);
		// fclose(fp10);
		// fclose(fp11);
		// fclose(fp12);
		// // fclose(fp13);
		// // fclose(fp14);
		// // fclose(fp15);
		// // fclose(fp16);
		// // fclose(fp17);
		// // fclose(fp18);
		// fclose(fp19);
		// fclose(fp20);
		// fclose(fp21);
	}
	
	delete[] comm;

	
	return 0;
}
