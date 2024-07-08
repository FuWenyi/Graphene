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
#include "util.h"

inline bool is_active(index_t vert_id,
					  sa_t criterion,
					  sa_t *sa, sa_t *prior)
{
	return (sa[vert_id] == criterion);
	//		return true;
	//	else
	//		return false;
}

bfs_result run_bfs(const int row_par, const int col_par, const int NUM_THDS, const char *beg_dir, const char *csr_dir,
				   const char *beg_header, const char *csr_header, const index_t num_chunks,
				   const size_t chunk_sz, const index_t io_limit, const index_t MAX_USELESS, const index_t ring_vert_count,
				   const index_t num_buffs, vertex_t root, sa_t *sa, index_t *comm, vertex_t **front_queue_ptr, index_t *front_count_ptr,
				   vertex_t *col_ranger_ptr, const index_t vert_count, const index_t vert_per_blk, sa_t *sa_dummy)
{

	// printf("begin BFS: %d\n",root);

	sa[root] = 0;
	IO_smart_iterator **it_comm = new IO_smart_iterator *[NUM_THDS];

	double tm = 0;
	double total_comp_time = 0;
	double total_io_time = 0;
	// double total_convert_time = 0; // （计算线程）填充bit map：根据front_queue中的active顶点确定这些点的邻居点，并将对应bit_map的值设置为1
	// double total_io_submit_time = 0; // (IO线程)
	// double total_poll_time = 0; // （IO线程）
	// double total_wait_io_time = 0; // （计算线程）BSP开始时计算线程等待IO线程第一个AIO返回数据
	// double total_wait_comp_time = 0; //(IO线程) Buffer用完时IO线程等待计算线程完成一个chunk计算的用时
	// double total_wait_aio_time = 0; // (IO线程)所有AIO用完时IO线程等待第一个返回的AIO的时间
	// // double total_comp_cache_time = 0; //（计算线程）计算cache部分，应该可以降低total_wait_io_time
	// // double total_translator_map_time1 = 0; // (计算线程) 填充bit_map时查询bitmap用时
	// // double total_translator_map_time2 = 0; // (计算线程) 填充bit_map时查询bitmap用时
	// // double total_comp_find_in_map_time = 0; // (计算线程) 计算cache中BFS时查询bitmap用时
	// // double total_comp_get_addr_map_time = 0; // (计算线程) 计算cache中BFS找到value用时
	// double total_comp_chunk_bfs_time = 0; // (计算线程) 计算chunk中BFS用时

	// // double total_get_cache_miss_time = 0;//为cache miss的点调用get预留空间的时间
	// // double total_put_cache_miss_time = 0;//将cache miss的点数据添加到lru的时间

	// long total_submitted_ctx_count = 0;  //（IO线程）总aio个数
	// long total_submitted_full_ctx_count = 0; //（IO线程）总req=64的aio个数
	// long total_submitted_req_count = 0;//（IO线程）总req个数
	// index_t total_reqt_blk_count = 0;

	// char cmd[256];
	// sprintf(cmd,"%s","iostat -x 1 -k > iostat_bfs.log&");
	// std::cout<<cmd<<"\n";
	// int total_level = 0;
#pragma omp parallel                        \
num_threads(NUM_THDS)                       \
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
		// if(tid < 16)
		//	pin_thread_socket(socket_one, 12);
		// else
		//	pin_thread_socket(socket_two, 12);

		index_t prev_front_count = 0;
		index_t front_count = 0;

		if ((tid & 1) == 0)
		{
			IO_smart_iterator *it_temp =
				new IO_smart_iterator(
					front_queue_ptr,
					front_count_ptr,
					col_ranger_ptr,
					comp_tid, comm,
					row_par, col_par,
					beg_dir, csr_dir,
					beg_header, csr_header,
					num_chunks,
					chunk_sz,
					sa, sa_dummy, beg_pos,
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
		IO_smart_iterator *it = it_comm[(tid >> 1) << 1];

		// if(!tid) system((const char *)cmd);
		// 记录io总时间
		//  double io_total_tm = 0;
		// 记录计算总时间
		//  double comp_total_tm = 0;
		//  double comp_tm_beg, comp_tm_end;
		// 记录总遍历的点数
		index_t total_vertex = 0;

		double convert_tm = 0;

		while (true)
		{
			front_count = 0;
			//- Framework gives user block to process
			//- Figures out what blocks needed next level
			if ((tid & 1) == 0)
			{
				it->io_time = 0;
				it->wait_io_time = 0;
				it->wait_comp_time = 0;
				it->cd->io_submit_time = 0;
				it->cd->io_poll_time = 0;
				it->cd->fetch_sz = 0;
				// it -> cd -> submitted_ctx_count = 0;
				// it -> cd -> submitted_req_count = 0;
				// it -> cd -> submitted_full_ctx_count = 0;
			}

			double comp_time = 0;
			double io_time = 0;
			// double comp_cache_time =0;
			// double comp_find_in_map_time = 0;
			// double comp_get_addr_map_time = 0;
			// double comp_chunk_bfs_time = 0;
			double temp_time;

			double ltm = wtime();

			if ((tid & 1) == 0)
			{
				it->is_bsp_done = false;
				// if((prev_front_count * 100.0)/ vert_count > 2.0)
				// 	it->req_translator(level);
				// else
				// {
				convert_tm = wtime();
				it->req_translator_queue();
				convert_tm = wtime() - convert_tm;
				// }
			}
			else
				it->is_io_done = false;

#pragma omp barrier
			if ((tid & 1) == 0) // 0为计算线程
			{
				// 记录本level的计算时间
				// fprintf(fp_nebr,"\n level = %d\n", level);
				comp_time = wtime();
				while (true)
				{
					int chunk_id = -1;
					double blk_tm = wtime();
					while ((chunk_id = it->cd->circ_load_chunk->de_circle()) == -1)
					{
						if (it->is_bsp_done)
						{
							chunk_id = it->cd->circ_load_chunk->de_circle();
							break;
						}
					}
					it->wait_io_time += (wtime() - blk_tm);

					if (chunk_id == -1)
						break;
					struct chunk *pinst = it->cd->cache[chunk_id];
					index_t blk_beg_off = pinst->blk_beg_off;
					index_t num_verts = pinst->load_sz;
					vertex_t vert_id = pinst->beg_vert;

					int head = it->cd->circ_load_chunk->head;

					// process one chunk
					//  temp_time = wtime();
					while (true)
					{
						if (sa[vert_id] == level)
						{
							index_t beg = beg_pos[vert_id - it->row_ranger_beg] - blk_beg_off;
							index_t end = beg + beg_pos[vert_id + 1 - it->row_ranger_beg] -
										  beg_pos[vert_id - it->row_ranger_beg];

							// possibly vert_id starts from preceding data block.
							// there by beg<0 is possible
							if (beg < 0)
								beg = 0;

							if (end > num_verts)
								end = num_verts;
							for (; beg < end; ++beg)
							{
								vertex_t nebr = pinst->buff[beg];
								if (sa[nebr] == INFTY)
								{
									sa[nebr] = level + 1;
									if (front_count <= it->col_ranger_end - it->col_ranger_beg)
										it->front_queue[comp_tid][front_count] = nebr;
									front_count++;
									// fprintf(fp_nebr,"%d\t%d\t%d\t%d\t%d\n", vert_id, sa[vert_id], nebr, sa[nebr],chunk_id);
								}
							}
						}
						++vert_id;

						if (vert_id >= it->row_ranger_end)
						{
							// fprintf(fp_nebr,"1. end_vert = %d, chunk id = %d; ",vert_id, chunk_id);
							break;
						}
						if (beg_pos[vert_id - it->row_ranger_beg] - blk_beg_off > num_verts)
						{
							// fprintf(fp_nebr,"2. end_vert = %d, chunk id = %d; ",vert_id, chunk_id);
							break;
						}
					}
					// comp_chunk_bfs_time += wtime() - temp_time;

					pinst->status = EVICTED;
					// fprintf(fp_nebr,"chunk status = %d\n", pinst->status);
					assert(it->cd->circ_free_chunk->en_circle(chunk_id) != -1);
				}

				it->front_count[comp_tid] = front_count;

				// 记录本level的计算时间
				comp_time = wtime() - comp_time;
			}
			else // 1为IO线程
			{
				io_time = wtime();
				while (it->is_bsp_done == false)
				{
					it->load_key(level);
					// it->load_key_iolist(level);
				}
				io_time = wtime() - io_time;
			}
		finish_point:
			comm[tid] = front_count;
#pragma omp barrier
			front_count = 0;
			for (int i = 0; i < NUM_THDS; ++i)
				front_count += comm[i];

			ltm = wtime() - ltm;
			if (!tid)
			{
				tm += ltm;
				total_comp_time += comp_time;
				total_comp_time += convert_tm;
				// total_convert_time += convert_tm;
				// total_wait_io_time += it->wait_io_time;
				// total_comp_cache_time += comp_cache_time;
				// total_translator_map_time1 += it->translator_map_time1;//暂未测
				// total_translator_map_time2 += it->translator_map_time2;
				// total_comp_find_in_map_time += comp_find_in_map_time;
				// total_comp_get_addr_map_time += comp_get_addr_map_time;
				// total_comp_chunk_bfs_time += comp_chunk_bfs_time;
				// total_get_cache_miss_time += comp_get_cache_miss_time;
				// total_put_cache_miss_time += comp_put_cache_miss_time;
			}
			else
			{
				total_io_time += io_time;
				// total_io_submit_time += it->cd->io_submit_time;
				// total_poll_time += it->cd->io_poll_time;
				// total_wait_comp_time += it->wait_comp_time;
				// // total_wait_aio_time += it->cd->wait_aio_time;
				// total_submitted_ctx_count += it->cd->submitted_ctx_count;
				// total_submitted_full_ctx_count += it->cd->submitted_full_ctx_count;
				// total_submitted_req_count += it->cd->submitted_req_count;
			}

#pragma omp barrier
			comm[tid] = it->cd->fetch_sz;
#pragma omp barrier
			index_t total_sz = 0;
			for (int i = 0; i < NUM_THDS; ++i)
				total_sz += comm[i];
			total_sz >>= 1; // total size doubled

			if (!tid)
			{
				std::cout << "@level-" << (int)level
						  << "-font-leveltime-converttm-iotm-waitiotm-waitcomptm-iosize: "
						  << front_count << " " << ltm << " " << convert_tm << " " << it->io_time
						  << "(" << it->cd->io_submit_time << "," << it->cd->io_poll_time << ") "
						  << " " << it->wait_io_time << " " << it->wait_comp_time << " "
						  << total_sz << "\n";

				// comp_total_tm += comp_tm_end;
				// io_total_tm += (it->cd->io_submit_time + it->cd->io_poll_time);

				// total_level = level;
			}

			if (front_count == 0 || level > 254)
				break;
			prev_front_count = front_count;

			total_vertex += front_count;

			front_count = 0;
			++level;
			// #pragma omp barrier
			// if(!tid) std::cout<<"\n\n\n";
			// #pragma omp barrier
		}
		// if(!tid)system("killall iostat");

		/*if(!tid){
			std::cout<<"Total time: "<<tm<<" second(s)\n";
			std::cout<<"Total io time: "<<io_total_tm<<" second(s)\n";
			std::cout<<"Total comp time: "<<comp_total_tm<<" second(s)\n";
			std::cout<<"Total vertex: "<<total_vertex<<"\n";

			FILE* fp_time = fopen("io_comp_time.txt", "a");
			if(fp_time == NULL) {
				printf("文件读取无效.\n");
			}
			fprintf(fp_time, "dataset:%s; root id = %d\n", beg_dir, root);
			fprintf(fp_time, "total_time\tio_time\tcomp_time\tio/comp\tvertex_num\tvertex_count\n");
			fprintf(fp_time, "%lf\t%lf\t%lf\t%lf\t%ld\t%ld\n\n", tm, io_total_tm, comp_total_tm, io_total_tm/comp_total_tm, total_vertex, vert_count);
			fclose(fp_time);
		} */

		if ((tid & 1) == 0)
			delete it;
	}

	// double cache_miss = 1;
	bfs_result result = bfs_result(tm, total_comp_time, total_io_time
								   //    total_convert_time, total_wait_io_time,
								   //    total_io_submit_time, total_poll_time, total_wait_comp_time, total_wait_aio_time,
								   //    total_comp_chunk_bfs_time, total_submitted_ctx_count,total_submitted_full_ctx_count,total_submitted_req_count
	);
	return result;
}
