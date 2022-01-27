#include "my_malloc.h"
size_t max(size_t a,size_t b){
    if(a>b){
        return a;
    }
    else{
        return b;
    }
}

void update_meta_data(meta_data * ptr,size_t status,
    size_t size,
    meta_data * prev,
    meta_data * next,
    footer * tail){
        ptr->status=status;
        ptr->size=size;
        ptr->prev=prev;
        ptr->next=next;
        ptr->tail=tail;
}
void update_footer_data(footer* ptr,size_t status,meta_data * head){
    ptr->head=head;
    ptr->status=status;
}

void init(){
    //initialize parameters and allocate a PAGE in heap
    free_head=NULL;
    heap_head=(void*)sbrk((intptr_t)PAGE_SIZE);
    heap_size=PAGE_SIZE;
    _init=1;
}
void add_node(meta_data* node){
    assert(node!=NULL);
    update_meta_data(node,node->status,node->size,NULL,free_head,node->tail);
    if(free_head){
        free_head->prev=node;
    }
    free_head=node;
    return;
}
void delete_node(meta_data * node){
    //delete a node and link its prev & next node together
    assert(node!=NULL&&node->status==FREE&&free_head);
    if(node->prev&&node->next){
        node->prev->next=node->next;
        node->next->prev=node->prev;
    }
    else if(node->prev){
        node->prev->next=NULL;
    }
    else if(node->next){
        node->next->prev=NULL;
        free_head=node->next;
    }
    else{
        free_head=NULL;
    }
}
meta_data * search_ff_free_node(size_t size){
    size_t required_size=size+FOOTER_SIZE+META_DATA_SIZE;
    meta_data* node=free_head;
    if(node==NULL){
        return NULL;
    }
    while(node!=NULL){
        if(node->size<required_size){
            node=node->next;
        }
        else{
            return node;
        }
    }
    return NULL;
}
meta_data * search_bf_free_node(size_t size){
    size_t required_size=size+FOOTER_SIZE+META_DATA_SIZE;
    meta_data * best_fit_node=NULL;
    int best_fit_size=heap_size+1;
    meta_data* node=free_head;
    if(node==NULL){
        return NULL;
    }
    while(node!=NULL){
        if(node->size==required_size){
            return node;
        }
        //if less than minimum larger node update
        if(node->size>required_size&&node->size<best_fit_size){
            best_fit_node=node;
            best_fit_size=node->size;
        }
        node=node->next;
    }
    if(best_fit_node){
        return best_fit_node;
    }
    return NULL;
}
void * extend_heap(size_t required_size,size_t * extended_sz){
    size_t page_num=required_size/PAGE_SIZE+1;
    (*extended_sz)=page_num*PAGE_SIZE;
    heap_size+=(*extended_sz);
    return sbrk((intptr_t)(*extended_sz));
}
meta_data * merge_prev_node(meta_data* node){
    assert(node&&node->status==FREE);
    footer * prev_tail=(footer*)((size_t)node-FOOTER_SIZE);
    if((void*)prev_tail<heap_head||prev_tail->status!=CLEAN){
        return node;
    }
    meta_data* prev_node=prev_tail->head;
    //if previous node is clean and its status is free merge together
    if(prev_node->status==FREE){
        delete_node(node);
        footer * original_footer=prev_node->tail;
        update_meta_data(prev_node,FREE,prev_node->size+node->size,prev_node->prev,prev_node->next,node->tail);
        update_footer_data(node->tail,CLEAN,prev_node);
        update_meta_data(node,DIRTY,0,NULL,NULL,NULL);
        update_footer_data(original_footer,DIRTY,0);
        return prev_node;
    }
    else{
        return node;
    }
}
void * my_malloc(size_t size,size_t malloc_mode){
    if(!_init){
        init();
    }
    meta_data * node;
    if(malloc_mode==FIRST_FIT){
        node=search_ff_free_node(size);
    }
    else{
        node=search_bf_free_node(size);
    }
    if(node==NULL){
        //if no free node finded, extend heap and allocate memory
        size_t extended_sz;
        size_t required_size=size+FOOTER_SIZE+META_DATA_SIZE;
        node=(meta_data*)extend_heap(required_size,&extended_sz);
        footer * node_tail=(footer*)((size_t)node+extended_sz-FOOTER_SIZE);
        update_meta_data(node,FREE,extended_sz,NULL,NULL,node_tail);
        update_footer_data(node_tail,CLEAN,node);
        //add to free list and merge with its prev node
        add_node(node);
        node = merge_prev_node(node);
    }
    size_t malloced_size=size+FOOTER_SIZE+META_DATA_SIZE;
    if(node->size-malloced_size<=META_DATA_SIZE+FOOTER_SIZE){
        //if the remaining size of free node can not hold meta data & footer
        //allocate it to user
        delete_node(node);
        update_meta_data(node,MALLOCED,node->size,node->prev,node->next,node->tail);
        return (void*)((size_t)node+META_DATA_SIZE);
    }
    //calculate remaining size 
    meta_data * rmn_free_node=(meta_data * )((size_t)node+malloced_size);
    footer * rmn_free_footer=node->tail;
    //update meta & footer data of remaining free block 
    update_meta_data(rmn_free_node,FREE,node->size-malloced_size,node->prev,node->next,node->tail);
    update_footer_data(rmn_free_footer,CLEAN,rmn_free_node);
    meta_data * malloced_node=node;
    footer * malloced_footer=(footer * )((size_t)node+size+META_DATA_SIZE);
    //update meta & footer data of new allocated block 
    update_meta_data(malloced_node,MALLOCED,malloced_size,NULL,NULL,malloced_footer);
    update_footer_data(malloced_footer,CLEAN,malloced_node);
    if(rmn_free_node->prev){
        rmn_free_node->prev->next=rmn_free_node;
    }
    else{
        free_head=rmn_free_node;
    }
    if(rmn_free_node->next){
        rmn_free_node->next->prev=rmn_free_node;
    }
    return (void*)((size_t)malloced_node+META_DATA_SIZE);
}
void * ff_malloc(size_t size){
    return my_malloc(size,FIRST_FIT);
}
void * bf_malloc(size_t size){
    return my_malloc(size,BEST_FIT);
}
void my_free(void * ptr){
    if(!ptr){
        return;
    }
    //calculate meta data location
    meta_data* node=(meta_data*)((size_t)ptr-META_DATA_SIZE);
    footer * node_footer=node->tail;
    //change node status to FREE
    update_meta_data(node,FREE,node->size,NULL,NULL,node_footer);
    update_footer_data(node_footer,CLEAN,node);
    //add to free list
    add_node(node);
    //merge its prev & next node
    node = merge_prev_node(node);
    meta_data* next_node=(meta_data*)((size_t)node_footer+FOOTER_SIZE);
    if((size_t)next_node>=(size_t)heap_head+heap_size||next_node->status!=FREE){
        return;
    }
    else{
        node=merge_prev_node(next_node);
    }
    return;
}

void ff_free(void * ptr){
    my_free(ptr);
    return;
}
void bf_free(void * ptr){
    my_free(ptr);
    return;
}

unsigned long get_largest_free_data_segment_size(){
    unsigned long largest_free_size=0;
    meta_data * node=free_head;
    if(node==NULL){
        return 0;
    }
    while(node->next!=NULL){
        largest_free_size=max(largest_free_size,(node->size-META_DATA_SIZE-FOOTER_SIZE));
        node=node->next;
    }
    largest_free_size=max(largest_free_size,(node->size-META_DATA_SIZE-FOOTER_SIZE));
    return largest_free_size;
}
unsigned long get_total_free_size(){
    unsigned long total_free_size=0;
    meta_data * node=free_head;
    if(node==NULL){
        return 0;
    }
    while(node->next!=NULL){
        total_free_size+=(node->size-META_DATA_SIZE-FOOTER_SIZE);
        node=node->next;
    }
    total_free_size+=(node->size-META_DATA_SIZE-FOOTER_SIZE);
    return total_free_size;
}