#include "vm/swap.h"

void 
init_swap(size_t used_index, void* kaddr)
{
    swap_block = block_get_role(BLOCK_SWAP); 
    /* PROJECT 3: block_size 함수가 num of sector를 반환하기 때문에 PGSIZE가 아닌 PGSIZE/SECTOR SIZE로 나눠줘야 함*/
	swap_bitmap = bitmap_create(block_size(swap_block) / (PGSIZE / BLOCK_SECTOR_SIZE));
	// bitmap_set_all (swap_table, 0);
    bitmap_set_all (swap_bitmap, 0);
}

void 
swap_in(size_t used_index, void* kaddr)
{
    lock_acquire(&swap_lock); 
    int unit_sector = PGSIZE / BLOCK_SECTOR_SIZE; 
    // 
    for (int i = 0 ; i < unit_sector ; i++)
    {
        block_read (swap_block, used_index * unit_sector + i, kaddr + i * BLOCK_SECTOR_SIZE);
    }
    
    bitmap_set(swap_bitmap, used_index, 0); 
    lock_release(&swap_lock); 
}

size_t 
swap_out(void* kaddr)
{
    int begin_index = bitmap_scan_and_flip(swap_bitmap, 0, 1, 0); // 비트맵에서 값이 0인 연속 1칸을 찾아서 1으로 flip 
    if (begin_index == BITMAP_ERROR) { return -1; } 

    int unit_sector = PGSIZE / BLOCK_SECTOR_SIZE; 
    
    for (int i = 0 ; i < unit_sector ; i++)
    {
        block_write (swap_block, begin_index * unit_sector + i, kaddr + i * BLOCK_SECTOR_SIZE);
    }

    return begin_index;
}