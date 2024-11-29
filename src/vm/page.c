#include "vm/page.h"

// struct list frame_table;
// struct frame_table_entry *current_clock;

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
    if(hash_delete(sp_table, spe)==NULL) return false;

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
    frame = malloc(sizeof(struct frame_table_entry));

    frame->kadd = kadd;
    frame->t = thread_current();

    // lock_acquire(&frame_table_lock);
    list_push_back(&frame_table, &frame->f_elem);
    // lock_release(&frame_table_lock);

    return frame;
}

bool
load_file(void* kadd, struct sp_entry *spe)
{
    if(!file_read_at(spe->file, kadd, spe->read_bytes, spe->offset)) return false;
    unsigned check = file_read_at(spe->file, kadd, spe->read_bytes, spe->offset);
    if(check < spe->read_bytes) return false;

    memset(kadd+spe->read_bytes, 0, spe->zero_bytes);
    return true;
}

void
f_table_init(void) //1124
{
    list_init(&frame_table);
    current_clock = NULL;
}