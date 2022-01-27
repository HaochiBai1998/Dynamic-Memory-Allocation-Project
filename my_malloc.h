#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>

#define META_DATA_SIZE 40
#define FOOTER_SIZE 16
#define PAGE_SIZE 4096
struct _meta_data;
struct _footer;
/**
 * @brief meta_data struct stores the meta information of a block including
 *        the status,size, its previous/next free node in free list and a footer 
 *        pointer. It in the head of a data block.
 * 
 * @param status - every data block has its own status DIRTY/FREE/MALLOCED
 * @param size - the total size of a data block
 * @param prev - a pointer to the previous block in free list.
 * @param next - a pointer to the next block in free list.
 * @param tail - a pointer to the footer part of this block.
 */
struct _meta_data{
    size_t status;
    size_t size;
    struct _meta_data * prev;
    struct _meta_data * next;
    struct _footer * tail;
};
typedef struct _meta_data meta_data;
/**
 * @brief struct footer stores the information of a block, including the head of
 *        this node and the status. Footer always located in the tail of a block.
 * @param status - CLEAN/DIRTY indicates whether the data inside this block is clean
 * @param head - a pointer to the meta data part of this block
 */
struct _footer{
    size_t status;
    struct _meta_data * head;
};
typedef struct _footer footer;
/**
 * @brief free_head - the head node of free list.
 */
static meta_data *free_head;
/**
 * @brief heap_head - the beginning of heap.
 */ 
static void *heap_head;
static size_t heap_size;
static size_t _init=0;
enum{
    DIRTY,
    CLEAN,
    FREE,
    MALLOCED,
    FIRST_FIT,
    BEST_FIT
};

size_t max(size_t a,size_t b);
/**
 * @brief update_meta_data - a function to update the meta information
 *        of a block.
 * 
 * @param ptr - meta_data type pointer points to the meta data part of
 *        a block
 */
void update_meta_data(meta_data * ptr,size_t status,
    size_t size,
    meta_data * prev,
    meta_data * next,
    footer * tail);
/**
 * @brief update_footer_data - a function to update the footer 
 *        information of a block.
 * @param ptr - a pointer points to the footer part of a block
 */
void update_footer_data(footer* ptr,size_t status,meta_data * head);
void init();
/**
 * @brief add_node - a function to add a new node into a free list
 */
void add_node(meta_data* node);
/**
 * @brief delete_node - a function to delete a node of a free list
 */
void delete_node(meta_data * node);

/**
 * @brief search_ff_free_node is a searching function of first fit
 *        malloc. This function will find the first free node whose
 *        size is larger than input and return it. If non-exist, 
 *        return NULL
 * 
 * @param size - the size required
 * @return meta_data* - a pointer to a node in free list.
 */
meta_data * search_ff_free_node(size_t size);
/**
 * @brief search_bf_free_node is a searching function of best fit
 *        malloc. This function will iterate through the whole list
 *        unless find the perfect fitted free node in a free list.
 *        Return the node with minimum size greater than user needed.
 * 
 * @param size - the size required
 * @return meta_data* - a pointer to a node in free list.
 */
meta_data * search_bf_free_node(size_t size);
/**
 * @brief extend_heap extends the size of heap according to the input
 *        required_size, the unit of extention is PAGE(4096 Byte).
 *        the actual extended size will be stored in extended_sz
 * @param required_size - the size required
 * @param extended_sz - actual allocated size
 * @return a pointer to the start of new allocated heap.
 **/
void * extend_heap(size_t required_size,size_t * extended_sz);

/**
 * @brief merge_prev_node merges the input node with its previous
 *        node if its previous node is FREE node. After merging
 *        process, free list will be updated, the footer of its
 *        previous block& the meta data of this input block will
 *        be erased
 * @param node - the node to be merged
 * @return a pointer to the new merged free node.
 **/
meta_data * merge_prev_node(meta_data* node);
/**
 * @brief By calling my_malloc, user can choose FIRST_FIT or BEST_FIT
 *        mode when user want to allocate memory.
 * @param size - allocation size required
 * @return a pointer to the new malloced block head.
 **/
void * my_malloc(size_t size,size_t malloc_mode);

/**
 * @brief ff_malloc is the implementation of first fit malloc, this function 
 *        will find the first node with size larger than input size and 
 *        return to the user.
 * @param size - allocation size required
 * @return a pointer to the new malloced block head.
 **/

void * ff_malloc(size_t size);
/**
 * @brief ff_malloc is the implementation of best fit malloc, this function 
 *        will find the perfectly fitted node with minimum size larger than
 *        required size
 * @param size - allocation size required
 * @return a pointer to the new malloced block head.
 **/
void * bf_malloc(size_t size);

/**
 * @brief my_free & ff_free & bf_free deallocates the input ptr block, and 
 *        merge its previous block & its next block
 * @param ptr - a pointer to the block to free.
 **/
void my_free(void * ptr);
void ff_free(void * ptr);
void bf_free(void * ptr);

unsigned long get_largest_free_data_segment_size();
unsigned long get_total_free_size();