#ifndef __CACHE_DRIVER__
#define __CACHE_DRIVER__

#include "util.h"
#include "comm.h"
#include "circle.h"

class cache_driver
{
	public:
		struct chunk **cache;	
		struct io_req **io_list;
		size_t chunk_sz;
		index_t num_chunks;
		struct timespec *time_out;
		int fd_csr;
		double io_submit_time;
		double io_poll_time;
		double io_assemble_time;
		double submit_io_req_time;
		double prep_pread_time;
		double io_submit__time;
		double io_getevents_time;
		int io_submit_num;
		int io_submit_16_num[4];

		index_t load_blk_off;
		index_t coarse_grain_off;
		bit_t *reqt_blk_bitmap;
		index_t *reqt_list;
		index_t *reqt_blk_count;
		vertex_t *blk_beg_vert;
		index_t total_blks;
		bool *io_conserve;
		index_t io_limit;	
		
		index_t VERT_PER_BLK;
		index_t vert_per_chunk;
		vertex_t *buff;
		struct io_event *events;
		struct iocb **piocb;
	
		index_t MAX_USELESS;

		circle *circ_free_ctx;
		circle *circ_free_chunk;
		circle *circ_load_chunk;
		circle *circ_submitted_ctx;
		index_t blk_per_chunk;
		long fetch_sz;
		long save_sz;

		// //add
		// long submitted_ctx_count;//提交io数
		// long submitted_req_count;//request总数
		// long submitted_full_ctx_count;//满64个request的总数

		// PM
		int io_fd;
		FILE *map_fd;
		std::map<int, IntPair> myfileMap;

	public:
		cache_driver(){};
		cache_driver(	
			int fd_csr,
			bit_t* &reqt_blk_bitmap,
			index_t *reqt_blk_count,
			const index_t total_blks,
			vertex_t *blk_beg_vert,
			bool *io_conserve,
			const index_t num_chunks,
			const size_t chunk_sz, 
			const index_t io_limit,
			index_t MAX_USELESS);
	
		cache_driver(	
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
			index_t MAX_USELESS);
	
		~cache_driver();

	public:
		
		//forked as another thread 
		//always clean out the avail chunks
		void clean_caches();
		
		//caller: IO_smart_iterator
		//blocked to issue the IO request
		
		//construct chunk at runtime
		void load_chunk();
		
		//construct chunk at runtime
		void load_chunk_iolist();
		
		//load all data in memory
		void load_chunk_full();

		//caller: IO_smart_iterator
		circle* get_chunk();

		void submit_io_req(index_t chunk_id);
};

#endif
