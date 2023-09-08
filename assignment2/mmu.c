#include "mmu.h"
#include <stdlib.h>
#include <stdio.h>


// byte addressable memory
unsigned char RAM[RAM_SIZE];  


// OS's memory starts at the beginning of RAM.
// Store the process related info, page tables or other data structures here.
// do not use more than (OS_MEM_SIZE: 72 MB).
unsigned char* OS_MEM = RAM;  

// memory that can be used by processes.   
// 128 MB size (RAM_SIZE - OS_MEM_SIZE)
unsigned char* PS_MEM = RAM + OS_MEM_SIZE; 


// This first frame has frame number 0 and is located at start of RAM(NOT PS_MEM).
// We also include the OS_MEM even though it is not paged. This is 
// because the RAM can only be accessed through physical RAM addresses.  
// The OS should ensure that it does not map any of the frames, that correspond
// to its memory, to any process's page. 
int NUM_FRAMES = ((RAM_SIZE) / PAGE_SIZE);

// Actual number of usable frames by the processes.
int NUM_USABLE_FRAMES = ((RAM_SIZE - OS_MEM_SIZE) / PAGE_SIZE);

// To be set in case of errors. 
int error_no; 



void os_init() {
    // TODO student 
    // initialize your data structures.
    for (int ind = 0; ind < 400 * PAGE_SIZE; ind++){
        OS_MEM[ind] = 0;
    }

    OS_MEM[0] = 2;
    OS_MEM[1] = 1;
}


// ----------------------------------- Functions for managing memory --------------------------------- //

/**
 *  Process Virtual Memory layout: 
 *  ---------------------- (virt. memory start 0x00)
 *        code
 *  ----------------------  
 *     read only data 
 *  ----------------------
 *     read / write data
 *  ----------------------
 *        heap
 *  ----------------------
 *        stack  
 *  ----------------------  (virt. memory end 0x3fffff)
 * 
 * 
 *  code            : read + execute only
 *  ro_data         : read only
 *  rw_data         : read + write only
 *  stack           : read + write only
 *  heap            : (protection bits can be different for each heap page)
 * 
 *  assume:
 *  code_size, ro_data_size, rw_data_size, max_stack_size, are all in bytes
 *  code_size, ro_data_size, rw_data_size, max_stack_size, are all multiples of PAGE_SIZE
 *  code_size + ro_data_size + rw_data_size + max_stack_size < PS_VIRTUAL_MEM_SIZE
 *  
 * 
 *  The rest of memory will be used dynamically for the heap.
 * 
 *  This function should create a new process, 
 *  allocate code_size + ro_data_size + rw_data_size + max_stack_size amount of physical memory in PS_MEM,
 *  and create the page table for this process. Then it should copy the code and read only data from the
 *  given `unsigned char* code_and_ro_data` into processes' memory.
 *   
 *  It should return the pid of the new process.  
 *  
 */
int create_ps(int code_size, int ro_data_size, int rw_data_size,
                 int max_stack_size, unsigned char* code_and_ro_data) 
{   
    // TODO student
    int new_pid = OS_MEM[0];
    OS_MEM[new_pid] = 1;

    unsigned char next_pid = new_pid;
    while (next_pid <= 1 || OS_MEM[next_pid] == 1){ 
        next_pid++;
    }
    OS_MEM[0] = next_pid;

    allocate_pages(new_pid, 0, code_size / PAGE_SIZE, O_READ | O_EX | O_WRITE);
    allocate_pages(new_pid, code_size, ro_data_size / PAGE_SIZE, O_READ | O_WRITE);
    allocate_pages(new_pid, code_size + ro_data_size, rw_data_size / PAGE_SIZE, O_READ | O_WRITE);
    allocate_pages(new_pid, 0x3fffff - max_stack_size + 1, max_stack_size / PAGE_SIZE, O_READ | O_WRITE);


    for(int addr = 0; addr < code_size + ro_data_size; addr++){
        write_mem(new_pid, addr, code_and_ro_data[addr]);
    }


    page_table_entry *page_table_start = (page_table_entry*) OS_MEM + (19 + new_pid) * PAGE_SIZE;
    page_table_entry *code_ptr = page_table_start;
    page_table_entry *ro_data_ptr = page_table_start + code_size / PAGE_SIZE;
    page_table_entry *rw_data_ptr = page_table_start + (code_size + ro_data_size) / PAGE_SIZE;
    
    while (code_ptr != ro_data_ptr) {
        modify_flags(code_ptr, O_READ | O_EX);
        code_ptr++;
    }
    while (ro_data_ptr != rw_data_ptr) {
        modify_flags(ro_data_ptr, O_READ);
        ro_data_ptr++;
    }
    
    return new_pid;
}

/**
 * This function should deallocate all the resources for this process. 
 * 
 */
void exit_ps(int pid) 
{
    // TODO student
    unsigned char *ppn_to_pid = OS_MEM + (2 * PAGE_SIZE);

    page_table_entry *page_table_start = (page_table_entry*) OS_MEM + (19 + pid) * PAGE_SIZE;

    for (int page = 0; page < 1024; page++){
        page_table_entry pte = page_table_start[page];
        int ppn = pte_to_frame_num(pte);

        unsigned char ppid = ppn_to_pid[ppn];
        *(page_table_start + page) = 0;

        if(ppid == pid){
            *(ppn_to_pid + ppn) = 0;
        }
    }

    OS_MEM[pid] = 0;
    OS_MEM[0] = pid;

}



/**
 * Create a new process that is identical to the process with given pid. 
 * 
 */
int fork_ps(int pid) {

    // TODO student:

    int new_pid = OS_MEM[0];
    OS_MEM[new_pid] = 1;

    unsigned char next_pid = new_pid;
    while (next_pid <= 1 || OS_MEM[next_pid] == 1){ 
        next_pid++;
    }
    OS_MEM[0] = next_pid;

    int copy_addr = 0x00;
    page_table_entry *page_table_start = (page_table_entry*) OS_MEM + (19 + new_pid) * PAGE_SIZE;
    page_table_entry *par_page_table_start = (page_table_entry*) OS_MEM + (19 + pid) * PAGE_SIZE;

    while (copy_addr <= 0x3fffff){

        if (copy_addr % PAGE_SIZE == 0){

            page_table_entry par = *(par_page_table_start + ((copy_addr & 0xfffff000) >> 12));

            if (is_present(par) == 0) {
                copy_addr += PAGE_SIZE;
                continue;
            }

            allocate_pages(new_pid, copy_addr, 1, O_READ | O_WRITE | O_EX);
        }

        unsigned char byte = read_mem(pid, copy_addr);
        write_mem(new_pid, copy_addr, byte);

        copy_addr++;
    }

    for (int num = 0; num < 1024; num++){

        modify_flags(page_table_start + num, (*(par_page_table_start + num)) & 0x7);

    }

    return new_pid;
    return 0;
}



// dynamic heap allocation
//
// Allocate num_pages amount of pages for process pid, starting at vmem_addr.
// Assume vmem_addr points to a page boundary.  
// Assume 0 <= vmem_addr < PS_VIRTUAL_MEM_SIZE
//
//
// Use flags to set the protection bits of the pages.
// Ex: flags = O_READ | O_WRITE => page should be read & writeable.
//
// If any of the pages was already allocated then kill the process, deallocate all its resources(ps_exit) 
// and set error_no to ERR_SEG_FAULT.
void allocate_pages(int pid, int vmem_addr, int num_pages, int flags) 
{
    // TODO student
    unsigned char *ppn_to_pid = OS_MEM + (2 * PAGE_SIZE);

    page_table_entry *page_table_start = (page_table_entry*) OS_MEM + (19 + pid) * PAGE_SIZE;
    int start_pte_num = (vmem_addr & 0xfffff000) >> 12;

    int cand_ppn = (OS_MEM_SIZE / PAGE_SIZE);

    for (int num = 0; num < num_pages; num++){
        page_table_entry pte = page_table_start[start_pte_num + num];
        
        if(is_present(pte) == 1){
            exit_ps(pid);
            error_no = ERR_SEG_FAULT;
            return;
        }

        while (cand_ppn < NUM_FRAMES && ppn_to_pid[cand_ppn] != 0){
            cand_ppn++;
        }

        if (cand_ppn >= NUM_FRAMES){
            exit_ps(pid);
            error_no = ERR_SEG_FAULT;
            return;
        }

        *(ppn_to_pid + cand_ppn) = pid;
        *(page_table_start + start_pte_num + num) = (cand_ppn << 8) | (1 << 3) | flags;
    }
}



// dynamic heap deallocation
//
// Deallocate num_pages amount of pages for process pid, starting at vmem_addr.
// Assume vmem_addr points to a page boundary
// Assume 0 <= vmem_addr < PS_VIRTUAL_MEM_SIZE

// If any of the pages was not already allocated then kill the process, deallocate all its resources(ps_exit) 
// and set error_no to ERR_SEG_FAULT.
void deallocate_pages(int pid, int vmem_addr, int num_pages) 
{
    // TODO student
    unsigned char *ppn_to_pid = OS_MEM + (2 * PAGE_SIZE);

    page_table_entry *page_table_start = (page_table_entry*) OS_MEM + (19 + pid) * PAGE_SIZE;
    int start_pte_num = (vmem_addr & 0xfffff000) >> 12;
    
    for(int num = 0; num < num_pages; num++){
        page_table_entry pte = page_table_start[start_pte_num + num];

        if (is_present(pte) == 0){
            exit_ps(pid);
            error_no = ERR_SEG_FAULT;
            return;
        }

        int frame_num = pte_to_frame_num(pte);
        unsigned char *ppid = ppn_to_pid + frame_num;

        if (*ppid != pid){
            exit_ps(pid);
            error_no = ERR_SEG_FAULT;
            return;
        }

        *ppid = 0;
        *(page_table_start + start_pte_num + num) = 0;

    }
}

// Read the byte at `vmem_addr` virtual address of the process
// In case of illegal memory access kill the process, deallocate all its resources(ps_exit) 
// and set error_no to ERR_SEG_FAULT.
// 
// assume 0 <= vmem_addr < PS_VIRTUAL_MEM_SIZE
unsigned char read_mem(int pid, int vmem_addr) 
{
    // TODO: student
    page_table_entry *page_table_start = (page_table_entry*) OS_MEM + (19 + pid) * PAGE_SIZE;
    int pte_num = (vmem_addr & 0xfffff000) >> 12;
    int offset = (vmem_addr & 0x0fff);
    
    page_table_entry pte = page_table_start[pte_num];
    if (!is_present(pte)){
        exit_ps(pid);
        error_no = ERR_SEG_FAULT;
        return ' ';
    }

    int frame_num = pte_to_frame_num(pte);
    return RAM[((frame_num << 12) + offset)];
}

// Write the given `byte` at `vmem_addr` virtual address of the process
// In case of illegal memory access kill the process, deallocate all its resources(ps_exit) 
// and set error_no to ERR_SEG_FAULT.
// 
// assume 0 <= vmem_addr < PS_VIRTUAL_MEM_SIZE
void write_mem(int pid, int vmem_addr, unsigned char byte) 
{
    // TODO: student
    page_table_entry  *page_table_start = (page_table_entry*) OS_MEM + (19 + pid) * PAGE_SIZE;
    int pte_num = (vmem_addr & 0xfffff000) >> 12;
    int offset = (vmem_addr & 0x0fff);
    
    page_table_entry pte = page_table_start[pte_num];
    if (is_present(pte) == 0 || is_writeable(pte) == 0){
        exit_ps(pid);
        error_no = ERR_SEG_FAULT;
        return;
    }

    int frame_num = pte_to_frame_num(pte);
    RAM[((frame_num << 12) + offset)] = byte;

}


void modify_flags(page_table_entry *ptr, int flags){
    page_table_entry pte = *ptr;
    pte = pte & 0xfffffff8;
    pte = pte | flags;
    *ptr = pte;
}



// ---------------------- Helper functions for Page table entries ------------------ // 

// return the frame number from the pte
int pte_to_frame_num(page_table_entry pte) 
{
    // TODO: student
    int ppn_shift = 8;
    int ppn_masked = pte & 0xffffff00;
    int ppn = ppn_masked >> ppn_shift;
    return ppn;
}


// return 1 if read bit is set in the pte
// 0 otherwise
int is_readable(page_table_entry pte) 
{
    // TODO: student
    return (pte & O_READ) >> 0;
    return 0;
}

// return 1 if write bit is set in the pte
// 0 otherwise
int is_writeable(page_table_entry pte) 
{
    // TODO: student
    return (pte & O_WRITE) >> 1;
    return 0;
}

// return 1 if executable bit is set in the pte
// 0 otherwise
int is_executable(page_table_entry pte) 
{
    // TODO: student
    return (pte & O_EX) >> 2;
    return 0;
}


// return 1 if present bit is set in the pte
// 0 otherwise
int is_present(page_table_entry pte) 
{
    // TODO: student
    return (pte & 0x8)>>3;
    return 0;
}

// -------------------  functions to print the state  --------------------------------------------- //

void print_page_table(int pid) 
{
    
    page_table_entry* page_table_start = NULL; // TODO student: start of page table of process pid
    int num_page_table_entries = -1;           // TODO student: num of page table entries

    page_table_start = (page_table_entry*) OS_MEM + (19 + pid) * PAGE_SIZE;
    num_page_table_entries = PAGE_SIZE / 4;

    // Do not change anything below
    puts("------ Printing page table-------");
    for (int i = 0; i < num_page_table_entries; i++) 
    {
        page_table_entry pte = page_table_start[i];
        printf("Page num: %d, frame num: %d, R:%d, W:%d, X:%d, P%d\n", 
                i, 
                pte_to_frame_num(pte),
                is_readable(pte),
                is_writeable(pte),
                is_executable(pte),
                is_present(pte)
                );
    }

}






