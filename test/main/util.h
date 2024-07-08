#ifndef __UTIL_H__
#define __UTIL_H__

typedef long index_t;
typedef unsigned vertex_t;
typedef unsigned char sa_t;
typedef unsigned char bit_t;
typedef unsigned int comp_t;

#define INFTY 				255

struct bfs_result {  // cache miss percent + total time
	double total_time;
	// double cache_miss_percent;
	double total_comp_time;
	double total_io_time;
	// double total_convert_time;
	// double total_wait_io_time;
	// // double total_comp_cache_time;
	// double total_io_submit_time;
	// double total_poll_time;
	// double total_wait_comp_time;
	// double total_wait_aio_time;
	// double total_comp_chunk_bfs_time;
	// // double total_translator_map_time1;
	// // double total_translator_map_time2;
	// // double total_comp_find_in_map_time;
	// // double total_comp_get_addr_map_time;

    // // double total_get_cache_miss_time;
	// // double total_put_cache_miss_time;

	// long total_submitted_ctx_count;  //（IO线程）总aio个数
	// long total_submitted_full_ctx_count; //（IO线程）总req=64的aio个数
	// index_t total_reqt_blk_count;



	bfs_result(double total_time, double total_comp_time, double total_io_time 
			/*,double total_convert_time, double total_wait_io_time,
			   double total_io_submit_time, double total_poll_time, double total_wait_comp_time, double total_wait_aio_time, double total_comp_chunk_bfs_time, 
			   long total_submitted_ctx_count, long total_submitted_full_ctx_count, index_t total_reqt_blk_count*/
			   ) {
		this->total_time = total_time;
		this->total_comp_time = total_comp_time;
		this->total_io_time = total_io_time;
		// this->total_convert_time = total_convert_time;
		// this->total_wait_io_time = total_wait_io_time;
		// this->total_io_submit_time = total_io_submit_time;
		// this->total_poll_time = total_poll_time;
		// this->total_wait_comp_time = total_wait_comp_time;
		// this->total_wait_aio_time = total_wait_aio_time;
		// this->total_comp_chunk_bfs_time = total_comp_chunk_bfs_time;
		// this->total_submitted_ctx_count = total_submitted_ctx_count;
		// this->total_submitted_full_ctx_count = total_submitted_full_ctx_count;
		// this->total_reqt_blk_count = total_reqt_blk_count;
	}
};

#endif
