#include "cache_driver.h"
#include <sys/mman.h>
#include <asm/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <stdint.h>
//bitmap + req_list together
cache_driver::cache_driver(
			int fd_csr,
			bit_t* &reqt_blk_bitmap,
			index_t* &reqt_list,
			index_t *reqt_blk_count,
			const index_t total_blks,
			vertex_t *blk_beg_vert,
			bool *io_conserve,
			const index_t num_chunks,
			const size_t chunk_sz,
			const index_t io_limit,
			index_t MAX_USELESS):
	cache_driver(fd_csr, reqt_blk_bitmap, reqt_blk_count,
			total_blks, blk_beg_vert, io_conserve, 
			num_chunks, chunk_sz, io_limit, MAX_USELESS)
{
	this->reqt_list = reqt_list;
}


//bitmap only implementation
cache_driver::cache_driver(
			int fd_csr,
			bit_t* &reqt_blk_bitmap,
			index_t *reqt_blk_count,
			const index_t total_blks,
			vertex_t *blk_beg_vert,
			bool *io_conserve,
			const index_t num_chunks,
			const size_t chunk_sz,
			const index_t io_limit,
			index_t MAX_USELESS):
	num_chunks(num_chunks),fd_csr(fd_csr),
	reqt_blk_count(reqt_blk_count),
	chunk_sz(chunk_sz),io_conserve(io_conserve),
	reqt_blk_bitmap(reqt_blk_bitmap),
	total_blks(total_blks),io_limit(io_limit),
	blk_beg_vert(blk_beg_vert),MAX_USELESS(MAX_USELESS)
{
	fetch_sz=0;

	io_submit_time = 0;
	io_poll_time = 0;
	VERT_PER_BLK=BLK_SZ/sizeof(vertex_t);
	load_blk_off = 0;
	coarse_grain_off = 0;
	vert_per_chunk = chunk_sz / sizeof(vertex_t);
	blk_per_chunk = vert_per_chunk/VERT_PER_BLK;

	//time out spec
	time_out = new struct timespec;
	time_out->tv_sec = 0;
	time_out->tv_nsec = 30000;
		
	//using a big buffer for all chunks
	buff=NULL;
	buff=(vertex_t *)mmap(NULL,chunk_sz*num_chunks,
		PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS 
		| MAP_HUGETLB | MAP_HUGE_2MB, 0, 0);
	if(buff==MAP_FAILED)
	{
		perror("buffer mmap");
		exit(-1);
	}
	
	//alloc buff space
	cache = new struct chunk*[num_chunks];
	for(index_t i=0;i<num_chunks;i++)
	{
		cache[i] = new struct chunk;
		cache[i]->load_sz = -1;
		cache[i]->blk_beg_off = -1;
		//try alignment of big array here
		cache[i]->status = EVICTED;
		cache[i]->buff = &(buff[i*vert_per_chunk]);
	}
	
	//alloc io list
	io_list = new struct io_req*[io_limit];
	piocb = new struct iocb*[MAX_EVENTS];
	for(index_t i=0;i<io_limit; ++i)
	{
		io_list[i] = new struct io_req;
		io_list[i]->io_cbp = new struct iocb[MAX_EVENTS];
		
		io_list[i]->num_ios=0;
		io_list[i]->ctx=0;
		io_list[i]->chunk_id = new index_t[MAX_EVENTS];
		if(io_setup(MAX_EVENTS,&(io_list[i]->ctx))!=0)
		{
			perror("io_setup error\n");
			exit(-2);
		}
	}

	events = new struct io_event[MAX_EVENTS];
	circ_free_chunk = new circle(num_chunks);
	circ_load_chunk = new circle(num_chunks);
	circ_free_ctx = new circle(io_limit);
	circ_submitted_ctx = new circle(io_limit);
	
	//every chunk should be en_circled into free chunk
	for(index_t i = 0; i < num_chunks; i++)
		circ_free_chunk->en_circle(i);
	
	for(index_t i = 0; i < io_limit; i++)
		circ_free_ctx->en_circle(i);
}

cache_driver::~cache_driver()
{
	munmap(buff,chunk_sz*num_chunks);
	for(index_t i=0;i<io_limit;i++)
	{
		if(io_destroy(io_list[i]->ctx)<0)
			perror("io_destroy");
		delete io_list[i]->io_cbp;
		delete io_list[i];
	}
	close(fd_csr);
	delete[] piocb;

	delete[] io_list;
	delete events; 
	delete circ_free_chunk;
	delete circ_load_chunk;
	delete circ_free_ctx;
	delete circ_submitted_ctx;
}

void cache_driver::submit_io_req(index_t io_id)
{
	int ret;
	assert(io_list[io_id]->num_ios<=MAX_EVENTS);
	
	//std::cout << io_list[io_id]->num_ios << "\n";
	io_submit_16_num[(io_list[io_id]->num_ios-1)/16] = io_submit_16_num[(io_list[io_id]->num_ios-1)/16] + 1;

	for(index_t i=0;i<io_list[io_id]->num_ios;i++)
	{
		//Record the address for submission
		piocb[i]=&(io_list[io_id]->io_cbp[i]);
		index_t chunk_id = io_list[io_id]->chunk_id[i];
		
		double before = wtime();
		#ifdef PM_MODE
		auto ele = myfileMap[cache[chunk_id]->blk_beg_off]; 
		io_prep_pread(&(io_list[io_id]->io_cbp[i]), 
				io_fd,
				cache[chunk_id]->buff, 
				ele.size*sizeof(vertex_t),
				ele.beg_blk*sizeof(vertex_t));			
		#else
		io_prep_pread(&(io_list[io_id]->io_cbp[i]), 
				fd_csr,
				cache[chunk_id]->buff, 
				cache[chunk_id]->load_sz*sizeof(vertex_t),
				cache[chunk_id]->blk_beg_off*sizeof(vertex_t));
		#endif
		prep_pread_time += wtime() - before;
		//if(pread(fd_csr, cache[chunk_id]->buff, 
		//			cache[chunk_id]->load_sz*sizeof(vertex_t),	
		//			cache[chunk_id]->blk_beg_off*sizeof(vertex_t))<=0)
		//{
		//	perror("pread");
		//	std::cout<<cache[chunk_id]->load_sz<<" "
		//		<<cache[chunk_id]->blk_beg_off<<"\n";
		//	exit(-1);
		//}
		
		fetch_sz += cache[chunk_id]->load_sz * sizeof(vertex_t);
	}

//	if(omp_get_thread_num() == 1)
//		std::cout<<"Submitted "<<cache[chunk_id]->beg_vert<<" "
//		<<cache[chunk_id]->load_sz<<" "
//		<<cache[chunk_id]->blk_beg_off*sizeof(vertex_t)
//		<<"\n";
//	
	double before = wtime();
	if((ret=io_submit(io_list[io_id]->ctx, 
				io_list[io_id]->num_ios, 
				piocb ))!=io_list[io_id]->num_ios)
	{
		perror("io_submit");
		printf("submitted: %d of %ld\n",ret,io_list[io_id]->num_ios);
		for(index_t i=0;i<io_list[io_id]->num_ios;i++)
		{
			index_t chunk_id = io_list[io_id]->chunk_id[i];
			std::cout<<"size-beg: "<<cache[chunk_id]->load_sz<<" "
				<<cache[chunk_id]->blk_beg_off<<"\n";
		}
		exit(-1);
	}
	//std::cout << "io submit time: " << wtime() - before;
	io_submit__time += wtime() - before;
	++ io_submit_num;
}


//for Bitmap based IO loading
void cache_driver::load_chunk()
{
	//Let cache driver do some work.
	//-If there is work to do
	//TODO!!! THIS IS A BIG ISSUE!
	//load_blk_off should be inited by someone else.
	double  this_time = wtime();

	/*FILE* fp = fopen("request_number.txt", "a");
	if(fp == NULL) {
		printf("文件读取无效.\n");
	}*/

	if(*io_conserve)
	{
		//printf("*reqt_blk_count: %ld\n", *reqt_blk_count);
		//- load_blk_off: next load will load from this block 
		load_blk_off = 0;
		*io_conserve = false;
		
		//reset circ_free_chunk
		circ_free_chunk->reset_circle();
		circ_load_chunk->reset_circle();
		//fprintf(fp, "reset load chunk, load_chunk_head = %d, free_chunk_head = %d\n", circ_load_chunk->head, circ_free_chunk->head);
		for(int i = 0; i < num_chunks; i ++)
		{
			//index_t beg_blk_id=cache[i]->blk_beg_off/VERT_PER_BLK;
			//index_t end_blk_id=beg_blk_id+cache[i]->load_sz/VERT_PER_BLK;
			//bool is_reuse=false;
			//while(beg_blk_id<end_blk_id)
			//{
			//	if(reqt_blk_bitmap[beg_blk_id>>3] & (1<<(beg_blk_id&7)))
			//	{
			//		if(!is_reuse) is_reuse = true;
			//		--(*reqt_blk_count);
			//		reqt_blk_bitmap[beg_blk_id>>3] &= (~(1<<(beg_blk_id&7)));	
			//	}
			//	beg_blk_id++;
			//}

			//if(is_reuse)
			//{
			//	cache[i]->status=LOADED;
			//	circ_load_chunk->en_circle(i);
			//}
			//else
			//{
			cache[i]->status=EVICTED;
			circ_free_chunk->en_circle(i);
			//}
		}
	}
	
	if(circ_free_ctx->get_sz()==0)
		return;
	
	index_t io_id = circ_free_ctx->de_circle();
	struct io_req *req = io_list[io_id];
	req->num_ios = 0;
	//fprintf(fp, "io_id = %d\n", io_id);
	
	if(load_blk_off >= total_blks && (*reqt_blk_count != 0))
	{
		//std::cout<<"Exhaust all blocks not finish all requests\n";
		//assert(*reqt_blk_count == 0);
		*reqt_blk_count = 0;

	}

	//std::cout<<"total_blks = "<<total_blks<<"; *reqt_blk_count = "<<*reqt_blk_count<<"\n";
	//记录每次request包含的blk数
	// int req_blk_num = 0;
	// double chunk_use;
	
	while((load_blk_off<total_blks) && (*reqt_blk_count != 0))
	{
		// req_blk_num = 0;
		//std::cout<<"total_blks = "<<total_blks<<"; *reqt_blk_count = "<<*reqt_blk_count<<"\n";
		//find a to-be-load block
		if(reqt_blk_bitmap[load_blk_off>>3] & (1<<(load_blk_off&7)))
		{
			//Get one free chunk
			//-record this chunk is submmited
			if(circ_free_chunk->get_sz() == 0)
			{
				//std::cout<<"Running out of chunk$$$$$$$$\n";
				//fprintf(fp,"free chunk size = 0\n");
				break;
			}
			
			index_t chunk_id = circ_free_chunk->de_circle();
			req->chunk_id[req->num_ios++] = chunk_id;

			//clean this bit
			--(*reqt_blk_count);
			reqt_blk_bitmap[load_blk_off>>3] &= (~(1<<(load_blk_off&7)));
			//每次request至少包含1个blk
			// ++req_blk_num ;
			// submitted_req_count++;

			//update chunk metadata
			cache[chunk_id]->status = LOADING;
			index_t beg_blk_id = load_blk_off;
			cache[chunk_id]->beg_vert= blk_beg_vert[beg_blk_id];
			cache[chunk_id]->blk_beg_off=VERT_PER_BLK * beg_blk_id;
			cache[chunk_id]->load_sz = VERT_PER_BLK;
			//打印每个chunk的开始点
			//fprintf(fp, "start request:\nbeg_vert = %d, chunk id = %d, chunk status = %d\n",cache[chunk_id]->beg_vert, chunk_id, cache[chunk_id]->status);

			//Loading is done
			if(*reqt_blk_count == 0)
			{
				//fprintf(fp,"1. reqt_blk_count = 0\n");
				goto finishpoint;
			}

			//Unite blocks to form a big chunk
			int continuous_useless_blk=0;			
			while(load_blk_off<total_blks)
			{
				++load_blk_off;
				if(load_blk_off+1-beg_blk_id>blk_per_chunk) break;
				
				if(reqt_blk_bitmap[load_blk_off>>3] & (1<<(load_blk_off&7)))
				{
					//continuous_useless_blk=0;
					// ++req_blk_num;
					--(*reqt_blk_count);
					reqt_blk_bitmap[load_blk_off>>3] &= (~(1<<(load_blk_off&7)));
					cache[chunk_id]->load_sz=(load_blk_off+1-beg_blk_id)*VERT_PER_BLK;
					
					//init load_blk_off for next level scanning
					if(*reqt_blk_count == 0)
					{
						//fprintf(fp,"2. reqt_blk_count = 0\n");
						goto finishpoint;
					}
				}
				else
				{
					// 如果 MAX_USELESS 个 block 内还没有有效 block，就停止探索
					continuous_useless_blk++;
					// ++req_blk_num;
					if(continuous_useless_blk==MAX_USELESS) break;
				}
			}
			//每执行完while生成一个request
			//std::cout<<"request id = "<<req->num_ios<<"; request blks = "<<req_blk_num<<"\n";
			// chunk_use = (double)req_blk_num/(double)blk_per_chunk;
			//fprintf(fp, "%d\t%d\t%d\t%d\t%lf\n", req->num_ios, req_blk_num, blk_per_chunk, chunk_id, chunk_use);
			//fprintf(fp, "request id = %d; reqt_blk_count = %d; request blks = %d; chunk_id = %d\n", req->num_ios, *reqt_blk_count, req_blk_num, chunk_id);
			// 每完成 MAX_EVENTS 个 io，就总体提交
			if(req->num_ios==MAX_EVENTS)
			{
				//avoid io_ctx miss tracking
				circ_submitted_ctx->en_circle(io_id);
				submit_io_req(io_id);
				// submitted_ctx_count++;
				// submitted_full_ctx_count++;

				//start a new io_ctx;
				if(circ_free_ctx->get_sz()==0)
				{
					io_submit_time += (wtime() - this_time);
					//fclose(fp);
					return;
				}

				io_id = circ_free_ctx->de_circle();
				req = io_list[io_id];
				req->num_ios=0;
			}
		}
		else	++load_blk_off;
	}
	
finishpoint:
	//avoid io_ctx miss tracking
	if(io_list[io_id]->num_ios != 0)
	{
		//最后一个request需要单独处理
		//std::cout<<"last request id = "<<req->num_ios<<"\n";
		//fprintf(fp, "request id = %d; reqt_blk_count = %d; request blks = %d\n", req->num_ios, *reqt_blk_count, req_blk_num);
		//req_blk_num = 0;
		circ_submitted_ctx->en_circle(io_id);
		submit_io_req(io_id);
		// submitted_ctx_count++;
	}
	else circ_free_ctx->en_circle(io_id);
	io_submit_time += (wtime() - this_time);

	//fprintf(fp,"finish load chunk\n");
	//fclose(fp);
	
	//if(load_blk_off>=total_blks)
	//{
	//	std::cout<<" I am done because I finished all blks???\n";
	//}
}


//Require the reqt-list is sorted
void cache_driver::load_chunk_iolist()
{
	//Let cache driver do some work.
	//-If there is work to do
	//TODO!!! THIS IS A BIG ISSUE!
	//load_blk_off should be inited by someone else.
	double  this_time = wtime();
	if(*io_conserve)
	{
		//printf("*reqt_blk_count: %ld\n", *reqt_blk_count);
		//- load_blk_off: next load will load from this block 
		load_blk_off = 0;
		*io_conserve = false;
		
		//reset circ_free_chunk
		circ_free_chunk->reset_circle();
		circ_load_chunk->reset_circle();
		for(int i = 0; i < num_chunks; i ++)
		{
			//index_t beg_blk_id=cache[i]->blk_beg_off/VERT_PER_BLK;
			//index_t end_blk_id=beg_blk_id+cache[i]->load_sz/VERT_PER_BLK;
			//bool is_reuse=false;
			//while(beg_blk_id<end_blk_id)
			//{
			//	if(reqt_blk_bitmap[beg_blk_id>>3] & (1<<(beg_blk_id&7)))
			//	{
			//		if(!is_reuse) is_reuse = true;
			//		--(*reqt_blk_count);
			//		reqt_blk_bitmap[beg_blk_id>>3] &= (~(1<<(beg_blk_id&7)));	
			//	}
			//	beg_blk_id++;
			//}

			//if(is_reuse)
			//{
			//	cache[i]->status=LOADED;
			//	circ_load_chunk->en_circle(i);
			//}
			//else
			//{
			cache[i]->status=EVICTED;
			circ_free_chunk->en_circle(i);
			//}
		}
	}
	
	if(circ_free_ctx->get_sz()==0)
		return;
	
	index_t io_id = circ_free_ctx->de_circle();
	struct io_req *req = io_list[io_id];
	req->num_ios = 0;

	while(load_blk_off < (*reqt_blk_count))
	{
		//Get one free chunk
		//-record this chunk is submmited
		if(circ_free_chunk->get_sz() == 0)
		{
			//std::cout<<"Running out of chunk$$$$$$$$\n";
			break;
		}

		index_t chunk_id = circ_free_chunk->de_circle();
		req->chunk_id[req->num_ios++] = chunk_id;

		//update chunk metadata
		//With the first to-be-loaded block
		cache[chunk_id]->status = LOADING;
		index_t beg_blk_id = reqt_list[load_blk_off];
		cache[chunk_id]->beg_vert = blk_beg_vert[beg_blk_id];
		cache[chunk_id]->blk_beg_off = VERT_PER_BLK * beg_blk_id;
		cache[chunk_id]->load_sz = VERT_PER_BLK;
		load_blk_off ++;
		
		//Unite blocks to form a big chunk
		while(load_blk_off < (*reqt_blk_count))
		{
			index_t blk_id = reqt_list[load_blk_off];
			if(blk_id < reqt_list[load_blk_off - 1]) break;
			if(blk_id + 1 - beg_blk_id > blk_per_chunk) break;
			if(blk_id - reqt_list[load_blk_off - 1] >= MAX_USELESS) break;
			cache[chunk_id]->load_sz = (blk_id+1-beg_blk_id)*VERT_PER_BLK;
		//	if(cache[chunk_id]->load_sz < 0) 
		//	{
		//		printf("blk_id %d, beg_blk_id %d, total %d\n", 
		//				blk_id, beg_blk_id, *reqt_blk_count);
		//	}

			load_blk_off ++;
		}
		
		//For IO iterator termination
		if(load_blk_off >= (*reqt_blk_count)) *reqt_blk_count = 0;

		if(req->num_ios == MAX_EVENTS)
		{
			//avoid io_ctx miss tracking
			circ_submitted_ctx->en_circle(io_id);
			submit_io_req(io_id);

			//start a new io_ctx;
			if(circ_free_ctx->get_sz()==0)
			{
				io_submit_time += (wtime() - this_time);
				return;
			}

			io_id = circ_free_ctx->de_circle();
			req = io_list[io_id];
			req->num_ios = 0;
		}
	}

	//avoid io_ctx miss tracking
	if(io_list[io_id]->num_ios != 0)
	{
		circ_submitted_ctx->en_circle(io_id);
		submit_io_req(io_id);
	}
	else circ_free_ctx->en_circle(io_id);
	io_submit_time += (wtime() - this_time);
		
}

//for Full IO
void cache_driver::load_chunk_full()
{
	//Let cache driver do some work.
	//-If there is work to do
	//TODO!!! THIS IS A BIG ISSUE!
	//load_blk_off should be inited by someone else.
	double  this_time = wtime();
	if(*io_conserve)
	{
		//printf("*reqt_blk_count: %ld\n", *reqt_blk_count);
		//- load_blk_off: next load will load from this block 
		load_blk_off = 0;
		*io_conserve = false;
		
		//reset circ_free_chunk
		circ_free_chunk->reset_circle();
		circ_load_chunk->reset_circle();
		for(int i = 0; i < num_chunks; i ++)
		{
			//index_t beg_blk_id=cache[i]->blk_beg_off/VERT_PER_BLK;
			//index_t end_blk_id=beg_blk_id+cache[i]->load_sz/VERT_PER_BLK;
			//bool is_reuse=false;
			//while(beg_blk_id<end_blk_id)
			//{
			//	if(reqt_blk_bitmap[beg_blk_id>>3] & (1<<(beg_blk_id&7)))
			//	{
			//		if(!is_reuse) is_reuse = true;
			//		--(*reqt_blk_count);
			//		reqt_blk_bitmap[beg_blk_id>>3] &= (~(1<<(beg_blk_id&7)));	
			//	}
			//	beg_blk_id++;
			//}

			//if(is_reuse)
			//{
			//	cache[i]->status=LOADED;
			//	circ_load_chunk->en_circle(i);
			//}
			//else
			//{
			cache[i]->status=EVICTED;
			circ_free_chunk->en_circle(i);
			//}
		}
	}
	
	if(circ_free_ctx->get_sz()==0)
		return;
	
	index_t io_id = circ_free_ctx->de_circle();
	struct io_req *req = io_list[io_id];
	req->num_ios = 0;

	while(load_blk_off < total_blks)
	{
		//Get one free chunk
		//-record this chunk is submmited
		if(circ_free_chunk->get_sz() == 0)
		{
			//std::cout<<"Running out of chunk$$$$$$$$\n";
			break;
		}

		index_t chunk_id = circ_free_chunk->de_circle();
		req->chunk_id[req->num_ios++] = chunk_id;

		//update chunk metadata
		//With the first to-be-loaded block
		cache[chunk_id]->status = LOADING;
		index_t beg_blk_id = load_blk_off;
		cache[chunk_id]->beg_vert = blk_beg_vert[beg_blk_id];
		cache[chunk_id]->blk_beg_off = VERT_PER_BLK * beg_blk_id;

		//Unite blocks to form a big chunk
		if((load_blk_off + blk_per_chunk) < total_blks)
		{
			load_blk_off += blk_per_chunk;
			cache[chunk_id]->load_sz = vert_per_chunk;
		}
		else
		{
			cache[chunk_id]->load_sz = (total_blks - beg_blk_id)*VERT_PER_BLK;
			load_blk_off = total_blks;
		}

		if(req->num_ios == MAX_EVENTS)
		{
			//avoid io_ctx miss tracking
			circ_submitted_ctx->en_circle(io_id);
			submit_io_req(io_id);

			//start a new io_ctx;
			if(circ_free_ctx->get_sz()==0)
			{
				io_submit_time += (wtime() - this_time);
				return;
			}


			io_id = circ_free_ctx->de_circle();
			req = io_list[io_id];
			req->num_ios = 0;
		}
	}
	
	//For IO iterator termination
	if(load_blk_off >= total_blks) *reqt_blk_count = 0;

	//avoid io_ctx miss tracking
	if(io_list[io_id]->num_ios != 0)
	{
		circ_submitted_ctx->en_circle(io_id);
		submit_io_req(io_id);
	}
	else circ_free_ctx->en_circle(io_id);
	io_submit_time += (wtime() - this_time);
		
}


//return a full circle of loaded chunks
circle *cache_driver::get_chunk()
{
	//std::cout<<"here\n";
	//Once entering here. 
	//-dump out at least half submissions
	//-if at half, still NULL, dump all
	int ret, n;
	const int check_limit = 8;
	int check_count = 0;
	double  this_time = wtime();
	/*FILE* fp = fopen("get_chunk_number.txt", "a");
	if(fp == NULL) {
		printf("文件读取无效.\n");
	}*/
	while(true)
	{

		index_t io_id = circ_submitted_ctx->de_circle();
		if(io_id == -1)
		{
			break;
		}
		//fprintf(fp, "io_id = %d\n", io_id);
		
		struct io_req *req=io_list[io_id];
//		std::cout<<"waiting io: "<<req->num_ios<<"\n";
		double before = wtime();
		ret=n=io_getevents(req->ctx,req->num_ios,MAX_EVENTS,
					events,NULL);
		io_getevents_time += wtime() - before;
//		std::cout<<"return waiting\n";
		if(ret!=req->num_ios)	
		{
			perror("io_getevents error\n");
			std::cout<<req->num_ios<<" "<<MAX_EVENTS<<"\n";
			continue;
		}

		for(int i=0;i<req->num_ios;i++)
		{
			//std::cout<<"Get-a-chunk\n";
			index_t chunk_id = req->chunk_id[i];
			cache[chunk_id]->status = LOADED;
			circ_load_chunk->en_circle(chunk_id);
			//fprintf(fp, "chunk_id = %d, load chunk tail = %d, tail id = %d\n", chunk_id, circ_load_chunk->tail, circ_load_chunk->array[circ_load_chunk->tail]);

			vertex_t vert_id = cache[chunk_id]->beg_vert;
			index_t num_verts = cache[chunk_id]->load_sz;
			//fprintf(fp, "get request:\nrequest id = %d; num_ios = %d; beg_vert = %d; chunk_id = %d; ", i, req->num_ios, vert_id, chunk_id);
			//fprintf(fp, "chunk status = %d\n", cache[chunk_id]->status);
		}
		
		req->num_ios=0;	
		circ_free_ctx->en_circle(io_id);
		
		//almost full
		if(circ_load_chunk->get_sz()
			>=circ_load_chunk->size-MAX_EVENTS) 
				break;

		check_count ++;
		if(check_count == check_limit) break;
	}
	
	io_poll_time += (wtime() - this_time);
	//fclose(fp);
	close(io_fd);
	return circ_load_chunk;
}

void cache_driver::clean_caches()
{
//	if(circ_free_chunk->get_sz() == 0)
	for(index_t i=0; i<num_chunks; i++)
	{
		if(cache[i]->status == PROCESSED)
		{
			//avoid enqueued again
			cache[i]->status = EVICTED;
			circ_free_chunk->en_circle(i);
		}
	}
}

void cache_driver::read_map(int level, int my_row, int my_col, int beg_header) {
	char useio_filename[256];
	char map_filename[256];
	cout << "level: " << level << " row: " << my_row << " col:" << my_col << " header: " << beg_header << "\n";
	sprintf(useio_filename, "%s/pm/io_level_%drow_%d_col_%d.bin", beg_header, level, my_row, my_col);
	sprintf(map_filename, "%s/pm/map_level_%drow_%d_col_%d.bin", beg_header, level, my_row, my_col);
	io_fd = open(useio_filename, O_RDONLY | O_DIRECT| O_NOATIME);
	if(io_fd == -1)
	{
		fprintf(stdout,"Wrong open %s\n", useio_filename);
		perror("open");
		exit(-1);
	}
	map_fd = fopen(map_filename, "rb");		// append binary mode
    if (map_fd == NULL) {
        perror("Failed to open file for reading");
        assert(0);
    }

	int data_to_read[3];
	// 读取数据块
	myfileMap.clear();
    while (fread(data_to_read, sizeof(int), 3, map_fd) == 3) {
        //printf("Read data: %d, %d, %d\n", data_to_read[0], data_to_read[1], data_to_read[2]);
		if (myfileMap.count(data_to_read[0])) {
			assert(0);
			std::cout << "wrong\n";
		} else {
			myfileMap[data_to_read[0]] = {data_to_read[1], data_to_read[2]};
		}
    }

    if (!feof(map_fd)) {
        perror("Error reading from map file");
        fclose(map_fd);
        assert(0);
    }
	fclose(map_fd);
}
