#include "util.h"
#include "comm.h"

#ifndef _H_GET_VERT_COUNT_
#define _H_GET_VERT_COUNT_

inline index_t get_vert_count(
		index_t *comm,
		const char *beg_dir,
		const char *deg_header,
		const int row_par,
		const int col_par
){
	char filename[256];
	index_t vert_count=0;
	for(int i=0; i < row_par; i++)
	{
		for(int j = 0; j < col_par; j ++)
		{
			sprintf(filename,"%s/row_%d_col_%d/%s.%d_%d_of_%dx%d.bin",
					beg_dir,i,j,deg_header,i,j,row_par,col_par);

			//printf("file: %s\n",filename);

			off_t fsz=fsize(filename);
			if(fsz==-1)
			{
				printf("%s wrong\n", filename);
			}

			//vertex's degree is equal to vert_count
			comm[i * col_par + j] = fsz / sizeof(index_t);
			vert_count += comm[i * col_par + j];
			std::cout << "thread[" << i * col_par + j << "] vert cnt: " << comm[i * col_par + j] << "\n";
		}
	}
	
	vert_count /= col_par;
	printf("Vertex count: %ld\n", vert_count);
	
	return vert_count;
}

#endif
