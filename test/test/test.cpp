#include <cstdio>
#include <assert.h>
#include <iostream>
#include <iomanip>
#define BUFFER_SIZE 0x200000

int main() {
    int col_ranger_ptr[2];
    char name[] = "/home/fuwenyi/ssd/pm/io_level0_row0_col0.bin";
    FILE *file = fopen(name, "wb");

    /*if(file != NULL) {
        printf("col-ranger file is found\n");
		size_t ret = fread(col_ranger_ptr, 4, 
				(1 + 1) * 1, file);
		assert(ret == (1 + 1) * 1);
		fclose(file);
        std::cout << col_ranger_ptr[0] << " " << col_ranger_ptr[1] << "\n";
    } */

    char *buffer = new char[BUFFER_SIZE];
    if (buffer == NULL) {
        perror("Error allocating memory");
        return 0; 
    }

    if (fwrite((char*)buffer, 1, 32175, file) != 32175) {
        perror("write");
    }
    std::cout << "Here1\n";
    //fclose(file);
    char name1[] = "/home/fuwenyi/ssd/pm/io_level1_row0_col0.bin";
    file = fopen(name1, "wb");
    if (fwrite((char*)buffer, 1, 3275, file) != 3275) {
        perror("write");
    }
    std::cout << "Here2\n";

    if (fwrite((char*)buffer, 1, 3275, file) != 3275) {
        perror("write");
    }
    std::cout << "Here3\n";

    if (fwrite((char*)buffer, 1, 3275, file) != 3275) {
        perror("write");
    }
    std::cout << "Here4\n";

    delete[] buffer;
    fclose(file);

    return 0;
}