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

//编译命令：g++ test_shmctx.cpp -g -o test_shmctx
int main(int argc, char **argv) 
{
    std::cout<<"shmid\n ";

    int csr_shmid = atoi(argv[1]);

    unsigned int *csr_shm_ptr;// = 0;
	csr_shm_ptr = (unsigned int *)shmat(csr_shmid, NULL, 0);
	if(csr_shm_ptr == (void*)-1){
		perror("csr_shmat: ");
		exit(-1);
	}

    for(int j = 0; j < 10; j++)
			printf("%u ", csr_shm_ptr[j]);
	    printf("\n");

}