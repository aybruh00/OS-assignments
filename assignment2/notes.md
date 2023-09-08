## PTE
pid(8) *|* ppn(16) *|* __(4) *\|* present(1) *\|* protection(3)  

os_mem[0] = next pid to be assigned  
os_mem[1:255] = is pid assigned / is program with pid existing

1 page for each process' pagetable


## Free list:
total number of pages available  
= number of entries in free list   
= 200 * 1024 * 1024  /  4 * 1024   
= 50 * 1024  
4 * 1024 entries in one page => 16 pages required  



