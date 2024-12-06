#include "vm/page.h"

// struct list frame_table;
// struct frame_table_entry *current_clock;

extern struct lock f_lock;

void 
sp_table_init(struct hash *sp_table)
{
    hash_init(sp_table, sp_hash_func, sp_less_func, NULL);
}

static unsigned
sp_hash_func(const struct hash_elem *e,void *aux)
{
    struct sp_entry *temp = hash_entry(e, struct sp_entry, elem);

    return hash_int((int)(temp->vaddr));
}

static bool 
sp_less_func (const struct hash_elem *a, const struct hash_elem *b)
{
    struct sp_entry *temp1 = hash_entry(a, struct sp_entry, elem);
    struct sp_entry *temp2 = hash_entry(b, struct sp_entry, elem);

    return (temp1->vaddr < temp2->vaddr);
}

bool 
insert_spe (struct hash *sp_table, struct sp_entry *spe)
{
    // 1124 
    bool result = false; 
    if (hash_insert(sp_table, &(spe->elem))==NULL) {
        result=true; 
    }
    return result; 
    //return hash_find(sp_table, spe)!=NULL;
}

bool 
delete_spe (struct hash *sp_table, struct sp_entry *spe)
{
    if(hash_delete(sp_table, &spe->elem)==NULL) return false;

    return true;
}

struct 
sp_entry *find_spe (void *vaddr)
{
    unsigned pg_num = pg_round_down(vaddr);
    struct sp_entry temp;
    temp.vaddr = pg_num;

    struct hash *table = &(thread_current()->sp_table);
    struct hash_elem *target = hash_find(table, &temp.elem);
    if(target!=NULL){
        return hash_entry(target, struct sp_entry, elem);
    }
    return NULL;
}

void 
spt_destroy (struct hash *sp_table)
{
    hash_destroy(sp_table, spt_destructor);
}

void
spt_destructor(struct hash_elem *elem, void *aux)
{
    struct sp_entry *temp = hash_entry(elem, struct sp_entry, elem);
    free(temp);
}

struct frame_table_entry *
frame_alloc(enum palloc_flags flag)
{
    struct frame_table_entry *frame;
    void* kadd;

    kadd = palloc_get_page(flag);
    /* project 3 1203 */
    if (kadd == NULL)
    {
       kadd = try_to_free_pages(flag); 
    }

    frame = malloc(sizeof(struct frame_table_entry));
    //printf("NO 1\n");
    frame->kadd = kadd;
    frame->t = thread_current();
    //printf("NO 2\n");
    // lock_acquire(&frame_table_lock);
    list_push_back(&frame_table, &frame->f_elem);
    // lock_release(&frame_table_lock);
    //printf("NO 3\n");
    return frame;
}

bool load_file(void * kaddr, struct sp_entry *spe) {
    bool flag = !lock_held_by_current_thread(&f_lock);
    size_t check;

    if(flag) lock_acquire(&f_lock);
    //file_seek(spe->file, spe->offset);
    //check = file_read(spe->file, kaddr, spe->read_bytes);
    check =  file_read_at(spe->file, kaddr, spe->read_bytes, spe->offset);
    if(flag) lock_release(&f_lock);

    if(check == spe->read_bytes) {
        memset(kaddr + check, 0, spe->zero_bytes);
        return true;
    }
    return false;
}
 
void
f_table_init(void) //1124
{
    list_init(&frame_table);
    current_clock = NULL;
}

// void 
// add_page_to_ft(struct frame_table_entry* page)
// {
//     list_push_back(&frame_table, &page->f_elem); 
// }

// void 
// del_page_from_ft(struct frame_table_entry* page)
// {
//     list_remove(&page->f_elem);
// }

void
free_page (void *kaddr)
{
    struct list_elem *fte_elem; 
    for(fte_elem=list_begin(&frame_table);fte_elem!=list_end(&frame_table);fte_elem=list_next(fte_elem))
        {
          struct frame_table_entry* fte_delete = list_entry(fte_elem, struct frame_table_entry, f_elem);
          if(fte_delete->kadd == kaddr){
            list_remove(&fte_delete->f_elem);
            palloc_free_page(kaddr);
            break;
          }
        }
}

static struct list_elem*
get_next_frame()
{   
    struct list_elem *element; 
    if(current_clock == NULL) 
    { 
        element = list_begin(&frame_table); 
        if(list_empty(&frame_table))
        {
            return NULL; 
        }
        if (element==NULL)
        {
            return NULL; 
        }
        
        if(element != list_end(&frame_table)) 
        { 
            current_clock = list_entry(element, struct frame_table_entry, f_elem); 
            return element; 
        } 
        else 
        {   
            return NULL; 
        }
    } 
    element = list_next(&current_clock->f_elem); 
    if(element == list_end(&frame_table)) 
    { 
        if(&current_clock->f_elem == list_begin(&frame_table)) 
        {   
            return NULL; }
        else 
            element = list_begin(&frame_table); 
    } 
    current_clock = list_entry(element, struct frame_table_entry, f_elem); 
    return element; 

    // struct list_elem *next; 
    // if (&current_clock->f_elem == NULL || !list_empty(&frame_table))
    // {
    //     next = list_begin(&frame_table);
    //     if (next == list_end(&frame_table)) { retuen NULL; } // 1205
    //     current_clock = list_entry(next, struct frame_table_entry, f_elem);
    //     return next;
    // }
    // // struct frame_table_entry *current_clock;
    // if (next == list_end(&frame_table))
    // {
    //     next = list_begin(&frame_table); 
    //     current_clock = list_entry(next, struct frame_table_entry, f_elem);
    //     return next; 
    // }

    // next = list_next(&current_clock->f_elem); 
    // current_clock = list_entry(next, struct frame_table_entry, f_elem);
    // return next; 
}

void*
try_to_free_pages(enum palloc_flags flags) /*evict 될 frame을 선택*/
{
 struct thread *page_thread; 
 struct list_elem *element; 
 struct frame_table_entry *lru_page; 
 if(list_empty(&frame_table) == true) 
 {  
    return palloc_get_page(flags); 
 } 

 while(true) 
 { 
  element = get_next_frame(); 
  if(element == NULL){ 
   return palloc_get_page(flags); 
  } 
  lru_page = list_entry(element, struct frame_table_entry, f_elem); 

  page_thread = lru_page->t; 
 
  if(pagedir_is_accessed(page_thread->pagedir, lru_page->spe->vaddr)) 
  { 
   pagedir_set_accessed(page_thread->pagedir, lru_page->spe->vaddr, false); 
   continue; 
  } 
 
  if(pagedir_is_dirty(page_thread->pagedir, lru_page->spe->vaddr) || lru_page->spe->type == VM_ANON) 
  { 
   if(lru_page->spe->type == VM_FILE) 
   { 
    file_write_at(lru_page->spe->file, lru_page->kadd ,lru_page->spe->read_bytes, lru_page->spe->offset); 
   } 
   else 
   { 
    lru_page->spe->type = VM_ANON; 
    lru_page->spe->swap_slot = swap_out(lru_page->kadd); 
   } 
  } 
  lru_page->spe->is_loaded = false; 
  pagedir_clear_page(page_thread->pagedir, lru_page->spe->vaddr); 
  palloc_free_page(lru_page->kadd); 
  //del_page_from_lru_list(lru_page); 
  if(lru_page!=NULL)
  {
    if (current_clock==lru_page)
    {
        current_clock = list_entry(list_remove(&lru_page->f_elem), struct frame_table_entry, f_elem);
    }
    else 
    {
        list_remove(&lru_page->f_elem);
    }
  }
  free(lru_page); 
  break; 
 } 
 return palloc_get_page(flags); 
}