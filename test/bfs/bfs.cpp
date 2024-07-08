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

//共享内存的实现
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 0x200000

inline bool is_active
(index_t vert_id,
sa_t criterion,
sa_t *sa, sa_t *prior)
{
	return (sa[vert_id]==criterion);
//		return true;
//	else 
//		return false;
}

int main(int argc, char **argv) 
{
	std::cout<<"Format: /path/to/exe " 
		<<"#row_partitions #col_partitions thread_count "
		<<"/path/to/beg_pos_dir /path/to/csr_dir "
		<<"beg_header csr_header num_chunks "
		<<"chunk_sz (#bytes) concurr_IO_ctx "
		<<"max_continuous_useless_blk ring_vert_count num_buffs source "
		<<"cache_sz(#bytes)\n";//指定存放csr的cache的大小(单位B)

	if(argc != 15)
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
	const index_t io_limit = atoi(argv[10]);			// 每个 IO 线程发起的 AIO 个数
	const index_t MAX_USELESS = atoi(argv[11]);
	const index_t ring_vert_count = atoi(argv[12]);
	const index_t num_buffs = atoi(argv[13]);
	vertex_t root = (vertex_t) atol(argv[14]);
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
	
	sa=(sa_t *)mmap(NULL,sizeof(sa_t)*vert_count,
			PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS 
			| MAP_HUGETLB | MAP_HUGE_2MB, 0, 0);
	//| MAP_HUGETLB , 0, 0);
	if(sa==MAP_FAILED)
	{	
		perror("mmap");
		exit(-1);
	}

	const index_t vert_per_blk = chunk_sz / sizeof(vertex_t);
	if(chunk_sz&(sizeof(vertex_t) - 1))
	{
		std::cout<<"Page size wrong\n";
		exit(-1);
	}
	sa_t *sa_dummy=NULL;
	
	//记录每次request
	remove("request_number.txt");

	char cmd[256];
	sprintf(cmd,"%s","iostat -x 1 -k -p > ./random_20_log/iostat_bfs.log&");
	std::cout<<cmd<<"\n";
	
	int *semaphore_acq = new int[1];
	int *semaphore_flag = new int[1];

	//	gpu_semaphore[0]=1;
	//omp_lock_t gpu_semaphore;
	//omp_init_lock(&gpu_semaphore);
	//0 1 2 3 4 5 6 7 8 9 10 11 12 13 28 29 30 31 32 33 34 35 36 37 38 39 40 41
	//14 15 16 17 18 19 20 21 22 23 24 25 26 27 42 43 44 45 46 47 48 49 50 51 52 53 54 55
	//int core_id[8]={0, 2, 4, 6, 14, 16, 18, 20};
	
	//int socket_one[12]={0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17};
	//int socket_two[12]={6, 7, 8, 9, 10, 11, 18, 19, 20, 21, 22, 23};
	int core_id[12]={0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17};
	int socket_one[12]={6, 7, 8, 9, 10, 11, 18, 19, 20, 21, 22, 23};
	int socket_two[12]={0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17};
	memset(sa, INFTY, sizeof(sa_t)*vert_count);
	sa[root] = 0;
	IO_smart_iterator **it_comm = new IO_smart_iterator*[NUM_THDS];
	
	double tm = 0;
	//int total_level = 0;
#pragma omp parallel \
	num_threads (NUM_THDS) \
	shared(sa, root, comm, front_queue_ptr, \
			front_count_ptr, col_ranger_ptr)
	{
		std::stringstream travss;
		travss.str("");
		travss.clear();

		std::stringstream fetchss;
		fetchss.str("");
		fetchss.clear();

		std::stringstream savess;
		savess.str("");
		savess.clear();

		sa_t level = 0;
		int tid = omp_get_thread_num();
		int comp_tid = tid >> 1;
		comp_t *neighbors;
		index_t *beg_pos;
		//if(tid < 16) 
		//	pin_thread_socket(socket_one, 12);
		//else
		//	pin_thread_socket(socket_two, 12);
		//pin_thread(core_id, tid);

		index_t prev_front_count = 0;
		index_t front_count = 0;
		index_t useful_sz = 0;
		
		if((tid&1) == 0) 
		{
			IO_smart_iterator *it_temp = 
				new IO_smart_iterator(
						front_queue_ptr,
						front_count_ptr,
						col_ranger_ptr,
						comp_tid,comm,
						row_par,col_par,	
						beg_dir,csr_dir, 
						beg_header,csr_header,
						num_chunks,
						chunk_sz,
						sa,sa_dummy,beg_pos,
						num_buffs,
						ring_vert_count,
						MAX_USELESS,
						io_limit,
						&is_active);

			front_queue_ptr[comp_tid][0] = root;
			front_count_ptr[comp_tid] = 1;

			prev_front_count = front_count_ptr[comp_tid];
			it_comm[tid] = it_temp;
			it_comm[tid]->is_bsp_done = false;
		}
#pragma omp barrier
		IO_smart_iterator *it = it_comm[(tid>>1)<<1];

		if(!tid) system((const char *)cmd);
		//记录io总时间
		double io_total_tm = 0;
		//记录计算总时间
		double comp_total_tm = 0;
		int total_io_submit_16_num[] = {0, 0, 0, 0};
		double comp_tm_beg, comp_tm_end;
		//记录总遍历的点数
		index_t total_vertex = 0;

		double convert_tm = 0;
		/*FILE* fp_nebr = fopen("level_nebr.txt", "a");
		if(fp_nebr == NULL) {
			printf("文件读取无效.\n");
		}*/

		while(true)
		{
			front_count = 0;
			useful_sz = 0;
			//- Framework gives user block to process
			//- Figures out what blocks needed next level
			if((tid & 1) == 0)
			{
				it -> io_time = 0;
				it -> wait_io_time = 0;
				it -> wait_comp_time = 0;
				it -> cd -> io_submit_time = 0;
				it -> cd -> io_poll_time = 0;
				it -> cd -> io_assemble_time = 0;
				it -> cd -> submit_io_req_time = 0;
				it -> cd -> prep_pread_time = 0;
				it -> cd -> io_submit__time = 0;
				it -> cd -> io_submit_num = 0;
				it -> cd -> io_getevents_time = 0;
				it -> cd -> fetch_sz = 0;
				for (int i = 0; i < 4; ++i)
					it -> cd -> io_submit_16_num[i] = 0;
			}

			double ltm=wtime();
			convert_tm=wtime();
			//std::cout << "[COMP] convert start time:" << convert_tm - start_tm;  
			if((tid & 1) == 0)
			{
				it->is_bsp_done = false;
				if((prev_front_count * 100.0)/ vert_count > 2.0) 
					it->req_translator(level);
				else
				{
					it->req_translator_queue();
				}
			}
			else it->is_io_done = false;
			//std::cout << "end time:" << wtime() - start_tm << "\n";
#pragma omp barrier
			convert_tm=wtime()-convert_tm;

			if((tid & 1) == 0)//0为计算线程
			{
				//记录本level的计算时间
				//fprintf(fp_nebr,"\n level = %d\n", level);
				// total_useful_sz 代表实际真正用到的边 IO
				comp_tm_beg =  wtime();
				
				// create useful io/map file for every level
				#ifdef PRE_MODE
				int my_row = comp_tid / col_par;
				int my_col = comp_tid % col_par;
				char useio_filename[256];
				char map_filename[256];
				sprintf(useio_filename, "%s/pm/io_level_%drow_%d_col_%d.bin", beg_header, level, my_row, my_col);
				sprintf(map_filename, "%s/pm/map_level_%drow_%d_col_%d.bin", beg_header, level, my_row, my_col);
				FILE *io_fd = fopen(useio_filename, "ab");		// append binary mode
				FILE *map_fd = fopen(map_filename, "ab");		// append binary mode
				int io_file_offset = 0;
				#endif

				while(true)
				{	
					int chunk_id = -1;
					double blk_tm = wtime();
					//std::cout << "[COMP] io wait start:" << blk_tm - start_tm;
					while((chunk_id = it->cd->circ_load_chunk->de_circle())
							== -1)
					{
						if(it->is_bsp_done)
						{
							chunk_id = it->cd->circ_load_chunk->de_circle();
							break;
						}
					}
					it->wait_io_time += (wtime() - blk_tm);
					//std::cout << "io wait end:" << wtime() - start_tm << "\n";

					if(chunk_id == -1) break;
					struct chunk *pinst = it->cd->cache[chunk_id];	
					index_t blk_beg_off = pinst->blk_beg_off;
					index_t num_verts = pinst->load_sz;
					vertex_t vert_id = pinst->beg_vert;

					int head = it->cd->circ_load_chunk->head;
					//fprintf(fp_nebr, "chunk_id = %d, load chunk head = %d, head id = %d\n", chunk_id, head, it->cd->circ_load_chunk->array[head]);
					//fprintf(fp_nebr,"beg_vert = %d, chunk id = %d, num_verts = %d, chunk status = %d\n", pinst->beg_vert, chunk_id, num_verts, pinst->status);

					// pre compute useful data
					#ifdef PRE_MODE
					void *buffer = malloc(BUFFER_SIZE);
					if (buffer == NULL) {
						perror("Error allocating memory");
						break;
					}
					int buffer_offset = 0;
					#endif

					//process one chunk
					while(true)
					{
						if(sa[vert_id] == level)
						{
							index_t beg = beg_pos[vert_id - it->row_ranger_beg] 
								- blk_beg_off;
							index_t end = beg + beg_pos[vert_id + 1 - 
								it->row_ranger_beg]- 
								beg_pos[vert_id - it->row_ranger_beg];

							//possibly vert_id starts from preceding data block.
							//there by beg<0 is possible
							if(beg<0) beg = 0;

							if(end>num_verts) end = num_verts;
							useful_sz += (end - beg) * sizeof(vertex_t);
							for( ;beg<end; ++beg)
							{
								vertex_t nebr = pinst->buff[beg];
								if(sa[nebr] == INFTY)
								{
									sa[nebr]=level+1;
									if(front_count <= it->col_ranger_end - it->col_ranger_beg)
										it->front_queue[comp_tid][front_count] = nebr;
									front_count++;
									//fprintf(fp_nebr,"%d\t%d\t%d\t%d\t%d\n", vert_id, sa[vert_id], nebr, sa[nebr],chunk_id);
								}
							}

							#ifdef PRE_MODE
							if (buffer_offset + useful_sz > BUFFER_SIZE) {
								perror("Error write to filr");
								free(buffer);
								buffer_offset = 0;
							}

							// append useful data to buffer
							memcpy(buffer + buffer_offset, &pinst->buff[beg], useful_sz);
							#endif
						}
						++ vert_id;

						if(vert_id >= it->row_ranger_end) {
							//fprintf(fp_nebr,"1. end_vert = %d, chunk id = %d; ",vert_id, chunk_id);
							break;
						}
						if(beg_pos[vert_id - it->row_ranger_beg]
								- blk_beg_off > num_verts) {
							//fprintf(fp_nebr,"2. end_vert = %d, chunk id = %d; ",vert_id, chunk_id);
							break;
						}
					}

					pinst->status = EVICTED;
					//fprintf(fp_nebr,"chunk status = %d\n", pinst->status);
					assert(it->cd->circ_free_chunk->en_circle(chunk_id)!= -1);

					// align buffer to block-grain and append it io to io-file and index to map-file
					#ifdef PRE_MODE
					int align_offset = (buffer_offset + (BLK_SZ - 1)) & ~(BLK_SZ - 1);
					if (align_offset > 0) {
        				if (fwrite(buffer, 1, align_offset, io_fd) != align_offset) {
            				perror("Error writing to io file");
        				}
    				} else {
						assert(0);
					}
					free(buffer);
					
					int data[3] = {blk_beg_off, io_file_offset, align_offset};
					if (fwrite(data, sizeof(int), 3, map_fd) != (sizeof(int) * 3)) {
            				perror("Error writing to map file");
        			}
					io_file_offset += align_offset;
					#endif
				}

				it->front_count[comp_tid] = front_count;

				//记录本level的计算时间
				comp_tm_end = wtime() - comp_tm_beg - it->wait_io_time;

				// close file
				fclose(io_fd);
				fclose(map_fd);
			}
			else //1为IO线程
			{
				while(it->is_bsp_done == false)
				{
					it->load_key(level);
					//it->load_key_iolist(level);
				}
			}
finish_point:	
			comm[tid] = front_count;
#pragma omp barrier
			front_count = 0;
			for(int i = 0 ;i< NUM_THDS; ++i)
				front_count += comm[i];

			ltm = wtime() - ltm;
			if(!tid) tm += ltm;

#pragma omp barrier
			comm[tid] = it->cd->fetch_sz;
#pragma omp barrier
			index_t total_sz = 0;
			for(int i = 0 ;i< NUM_THDS; ++i)
				total_sz += comm[i];
			total_sz >>= 1;//total size doubled

#pragma omp barrier
			comm[tid] = useful_sz;
#pragma omp barrier
			useful_sz = 0;
			for(int i = 0 ;i< NUM_THDS; ++i)
				useful_sz += comm[i];

			if(!tid){
				std::cout<<"@level-"<<(int)level
				<<"-font-leveltime-converttm-iotm-waitiotm-waitcomptm-iosize-usesize: "
				<<front_count<<" "<<ltm<<" "<<convert_tm<<" "<<it->io_time
				<<"("<<it->cd->io_submit_time<<","<<it->cd->io_poll_time<<") "
				<<" "<<it->wait_io_time<<" "<<it->wait_comp_time<<" "
				<<total_sz<<" "<<comp_tm_end<<" "<<it->cd->io_submit__time<<" "
				<<it->cd->io_submit_num<<" "<<it->cd->io_getevents_time<<" "<<useful_sz<<"\n";

				comp_total_tm += comp_tm_end;
				io_total_tm += (it->cd->io_submit_time + it->cd->io_poll_time);
				/*for (int i = 0; i < 4; ++i) {
					std::cout << it->cd->io_submit_16_num[i] << "\n";
					total_io_submit_16_num[i] += it->cd->io_submit_16_num[i];
				}*/
				
				//std::cout << "submit_io_req_time: " << it->cd->submit_io_req_time << "\n";
				//std::cout << "prep_pread_time: " << it->cd->prep_pread_time << "\n";
				//std::cout << "io_submit__time: " << it->cd->io_submit__time << "\n";
				//std::cout << "io_submit_num: " << it->cd->io_submit_num << "\n";
				//total_level = level;
				//std::cout << "total_useful_sz: " << useful_sz << "\n";
			}
			
			if(front_count == 0 || level > 254) break;
			prev_front_count = front_count;

			total_vertex += front_count;

			front_count = 0;
			++ level;
			++ it->my_level;
			assert(level == it->my_level);
//#pragma omp barrier
			//if(!tid) std::cout<<"\n\n\n";
//#pragma omp barrier
		}
		if(!tid)system("killall iostat");

		if(!tid){
			std::cout<<"Total time: "<<tm<<" second(s)\n";
			std::cout<<"Total io time: "<<io_total_tm<<" second(s)\n";
			std::cout<<"Total comp time: "<<comp_total_tm<<" second(s)\n";
			std::cout<<"Total vertex: "<<total_vertex<<"\n";
			std::cout<<"Total io(0-15): "<<total_io_submit_16_num[0]<<" "
					 <<"Total io(16-31): "<<total_io_submit_16_num[1]<<" "
					 <<"Total io(32-47): "<<total_io_submit_16_num[2]<<" "
					 <<"Total io(48-63): "<<total_io_submit_16_num[3]<<"\n";
		} 
		
		if((tid & 1) == 0) delete it;
	}
	
	munmap(sa,sizeof(sa_t)*vert_count);
	delete[] comm;

	//	gpu_freer(keys_d);	
	return 0;
}
