// add this to the mmu.c file and run
#include "mmu.h"
#include <stdio.h>
#include <assert.h>

#define MB (1024 * 1024)

#define KB (1024)

// just a random array to be passed to ps_create
unsigned char code_ro_data[10 * MB];

int error_no;


int test() 
{

	os_init();
    
	code_ro_data[10 * PAGE_SIZE] = 'c';   // write 'c' at first byte in ro_mem
	code_ro_data[10 * PAGE_SIZE + 1] = 'd'; // write 'd' at second byte in ro_mem

	int p1 = create_ps(10 * PAGE_SIZE, 1 * PAGE_SIZE, 2 * PAGE_SIZE, 1 * MB, code_ro_data);

	extern int error_no;
	error_no = -1;
	// int error_no = -1; // no error


    
	unsigned char c = read_mem(p1, 10 * PAGE_SIZE);

	assert(c == 'c');

	unsigned char d = read_mem(p1, 10 * PAGE_SIZE + 1);
	assert(d == 'd');

	assert(error_no == -1); // no error


	write_mem(p1, 10 * PAGE_SIZE, 'd');   // write at ro_data

	assert(error_no == ERR_SEG_FAULT);  


// my tests 

	// write_mem(p1, 11*PAGE_SIZE, 'p');

	// c = read_mem(p1, 11 * PAGE_SIZE);
	// assert(c == 'p');
	// // printf("Forking\n");
	// int p2 = fork_ps(p1);
	// // puts("forked");

	// c = read_mem(p1, 11 * PAGE_SIZE);
	// assert(c == 'p');
	// c = read_mem(p2, 11 * PAGE_SIZE);
	// assert(c == 'p');

	// error_no = -1;

	// write_mem(p2, 11*PAGE_SIZE, 'c');
	// c = read_mem(p1, 11 * PAGE_SIZE);
	// assert(c == 'p');
	// c = read_mem(p2, 11 * PAGE_SIZE);
	// assert(c == 'c');
	// write_mem(p1, 11*PAGE_SIZE, 'q');
	// c = read_mem(p1, 11 * PAGE_SIZE);
	// assert(c == 'q');
	// c = read_mem(p2, 11 * PAGE_SIZE);
	// assert(c == 'c');


	// assert(error_no == -1); // no error

// my tests end 


	int p2 = create_ps(1 * MB, 0, 0, 1 * MB, code_ro_data);	// no ro_data, no rw_data

	error_no = -1; // no error


	int HEAP_BEGIN = 1 * MB;  // beginning of heap

	// allocate 250 pages
	allocate_pages(p2, HEAP_BEGIN, 250, O_READ | O_WRITE);

	write_mem(p2, HEAP_BEGIN + 1, 'c');

	write_mem(p2, HEAP_BEGIN + 2, 'd');

	assert(read_mem(p2, HEAP_BEGIN + 1) == 'c');

	assert(read_mem(p2, HEAP_BEGIN + 2) == 'd');

	deallocate_pages(p2, HEAP_BEGIN, 10);

	print_page_table(p2); // output should atleast indicate correct protection bits for the vmem of p2.

	write_mem(p2, HEAP_BEGIN + 1, 'd'); // we deallocated first 10 pages after heap_begin

	assert(error_no == ERR_SEG_FAULT);


	int ps_pids[100];

	// requesting 2 MB memory for 64 processes, should fill the complete 128 MB without complaining.   
	for (int i = 0; i < 64; i++) {
    	ps_pids[i] = create_ps(1 * MB, 0, 0, 1 * MB, code_ro_data);
    	print_page_table(ps_pids[i]);	// should print non overlapping mappings.  
	}


	exit_ps(ps_pids[0]);
    

	ps_pids[0] = create_ps(1 * MB, 0, 0, 500 * KB, code_ro_data);

	print_page_table(ps_pids[0]);   

	// allocate 500 KB more
	allocate_pages(ps_pids[0], 1 * MB, 125, O_READ | O_READ | O_EX);

	for (int i = 0; i < 64; i++) {
    	print_page_table(ps_pids[i]);	// should print non overlapping mappings.  
	}
}

void main(){

	// print_page_table(1);
	// create_ps(0,0,0, 8*KB, code_ro_data);

	test();

	// os_init();

	// for(int i=0; i<64;i++){
	// 	int pid = create_ps(1*MB, 0, 0, 1*MB, code_ro_data);
	// 	printf("page table of pid %d\n", pid);
	// 	print_page_table(pid);
	// }

}