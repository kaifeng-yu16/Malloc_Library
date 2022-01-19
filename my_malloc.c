#include "my_malloc.h"

meta_data_t * block_head = NULL;
meta_data_t * block_tail = NULL;
meta_data_t * free_list_head = NULL;
meta_data_t * free_list_tail = NULL;
unsigned long segment_size = 0;
unsigned long segment_free_space_size = 0;

void *ff_malloc(size_t size) {
  return  f_malloc(size, &add_to_free_list_ff);
}

void ff_free(void *ptr) {
  f_free(ptr, &add_to_free_list_ff);
}

void *bf_malloc(size_t size) {
  return  f_malloc(size, &add_to_free_list_bf);
}

void bf_free(void *ptr) {
  f_free(ptr, &add_to_free_list_bf);
}

unsigned long get_data_segment_size() { // in bytes
  return segment_size;
}

unsigned long get_data_segment_free_space_size() { // in bytes
  return segment_free_space_size;
}

void * f_malloc(size_t size, add_func_t f) {
  if (size == 0) {
    return NULL;
  }
  void * addr = NULL;
  if ((addr = try_existed_block(size, f)) == NULL) {
    // require a new block
    void * ptr =add_new_block(size);
    if (ptr == (void *) -1) {
      return NULL;
    } else {
      return ptr + sizeof(meta_data_t);
    }
  }
  //assert(((meta_data_t *)addr)->is_used == 1);
  return addr + sizeof(meta_data_t);
}

void * try_existed_block(size_t size, add_func_t f) {
  meta_data_t * ptr = free_list_head;
  while (ptr != NULL) {
    //assert(ptr->is_used == 0);
    if (ptr->size >= size) {
      break;
    }
    ptr = ptr->next_free_block;
  }
  // no available free block
  if (ptr == NULL) {
    return NULL;
  }
  remove_block(ptr);
  // need to seperate into two blocks
  if (ptr->size > size + sizeof(meta_data_t)) {
    meta_data_t* block2 = split_block(ptr, size);
    f(block2);
  }
  return ptr;
}

void * add_new_block(size_t size) {
  void * ptr = sbrk(size + sizeof(meta_data_t));
  if (ptr == (void * )-1) {
    return ptr;
  }
  meta_data_t * block_meta = (meta_data_t *) ptr;
  block_meta->size = size;
  block_meta->is_used = 1;
  block_meta->prev_block = block_tail;
  if (block_tail != NULL) {
    block_tail->next_block = block_meta;
  } else {
    block_head = block_meta;
  }
  block_tail = block_meta;
  block_meta->next_block = NULL;
  block_meta->prev_free_block = NULL;
  block_meta->next_free_block = NULL;
  
  segment_size += (size + sizeof(meta_data_t));
  return ptr;
}

void f_free(void * ptr, add_func_t f) {
  if (ptr == NULL) {
    return;
  }
  //assert ((unsigned long)ptr >= sizeof(meta_data_t));
  meta_data_t * block = (meta_data_t *) (ptr - sizeof(meta_data_t));
  //assert(block->is_used == 1);
  try_coalesce(block, f);
}

void try_coalesce(meta_data_t * block, add_func_t f) {
  meta_data_t * new_block = block;
  // need to delete, not always true
  //assert(block->is_used == 1);
  if (block->is_used == 0) {
    remove_block(block);
  }
  //assert(block->is_used == 1);
  if (block->next_block != NULL &&
      block->next_block->is_used == 0 &&
      (void *)(block) + sizeof(meta_data_t) + block->size == block->next_block) {
    remove_block(block->next_block);
    block->size += (block->next_block->size + sizeof(meta_data_t));
    if (block->next_block->next_block != NULL) {
      block->next_block->next_block->prev_block = block;
    }
    block->next_block = block->next_block->next_block;
  }
  if (block->prev_block != NULL &&
      block->prev_block->is_used == 0 &&
      (void *)(block->prev_block) + sizeof(meta_data_t) + block->prev_block->size == block) {
    remove_block(block->prev_block);
    block->prev_block->size += (block->size + sizeof(meta_data_t));
    block->prev_block->next_block = block->next_block;
    if (block->next_block != NULL) {
      block->next_block->prev_block = block->prev_block;
    }
    new_block = block->prev_block;
  }
  f(new_block);
}

void add_to_free_list_ff(meta_data_t * block) {
  //assert(block->is_used == 1);
  //assert(block->prev_free_block == NULL && block->next_free_block == NULL);
  block->is_used = 0;
  meta_data_t ** ptr = &free_list_head;
  while ((*ptr) != NULL) {
    if (*ptr > block) {
      break;
    }
    ptr = &((*ptr)->next_free_block);
  }
  block->next_free_block = *ptr;
  if ((*ptr) != NULL) {
    block->prev_free_block = (*ptr)->prev_free_block;
    (*ptr)->prev_free_block = block;
  } else {
    block->prev_free_block = free_list_tail;
    free_list_tail = block;
  }
  (*ptr) = block;
  segment_free_space_size += block->size + sizeof(meta_data_t);
}

void add_to_free_list_bf(meta_data_t * block) {
  //assert(block->is_used == 1);
  //assert(block->prev_free_block == NULL && block->next_free_block == NULL);
  block->is_used = 0;
  meta_data_t ** ptr = &free_list_head;
  while ((*ptr) != NULL) {
    if ((*ptr)->size > block->size) {
      break;
    } else if ((*ptr)->size == block->size &&
        (*ptr) > block){
    }
    ptr = &((*ptr)->next_free_block);
  }
  block->next_free_block = *ptr;
  if ((*ptr) != NULL) {
    block->prev_free_block = (*ptr)->prev_free_block;
    (*ptr)->prev_free_block = block;
  } else {
    block->prev_free_block = free_list_tail;
    free_list_tail = block;
  }
  (*ptr) = block;
  segment_free_space_size += block->size + sizeof(meta_data_t);
}

void remove_block(meta_data_t * block) {
  block->is_used = 1;
  if (block->prev_free_block == NULL) {
    free_list_head = block->next_free_block;
  } else {
    block->prev_free_block->next_free_block = block->next_free_block;
  }
  if (block->next_free_block == NULL) {
    free_list_tail = block->prev_free_block;
  } else {
    block->next_free_block->prev_free_block = block->prev_free_block;
  }
  block->prev_free_block = NULL;
  block->next_free_block = NULL;
  segment_free_space_size -= block->size + sizeof(meta_data_t);
}

// can only split used block
meta_data_t * split_block(meta_data_t * block1, size_t size) {
  // important //assertion
  //assert(block1->is_used == 1);
  //assert(block1->prev_free_block == NULL && block1->next_free_block == NULL);
  //assert(block1->size - size > sizeof(meta_data_t));
  meta_data_t * block2 = (meta_data_t *)((void *)block1 + size + sizeof(meta_data_t));
  block2->is_used = block1->is_used;
  block2->size = block1->size - sizeof(meta_data_t) - size;
  block1->size = size;
  block2->prev_block = block1;
  block2->next_block = block1->next_block;
  if (block1->next_block != NULL) {
    block1->next_block->prev_block = block2;
  }
  block1->next_block = block2;
  block2->prev_free_block = NULL;
  block2->next_free_block = NULL;
  return block2;
}

void print_blocks() {
  printf("***Block Data***\n");
  int cnt = 0;
  meta_data_t * ptr = block_head;
  while (ptr != NULL) {
    printf("block %d: is_used[%lu], size[%lu], prev_block[%lu], next_block[%lu], \
        prev_free[%lu], next_free[%lu] \n", cnt, ptr->is_used, ptr->size,
        (unsigned long)ptr->prev_block, (unsigned long)ptr->next_block, (unsigned long)ptr->prev_free_block,
        (unsigned long)ptr->next_free_block);
    ptr = ptr->next_block;
    ++cnt;
  }
  printf("Total blocks: %d\n\n", cnt);
}

void print_free_list() {
  printf("***Free List Data***\n");
  int cnt = 0;
  meta_data_t * ptr = free_list_head;
  while (ptr != NULL) {
    printf("free block %d: is_used[%u], size[%lu], prev_block[%lu], next_block[%lu], \
      prev_free[%lu], next_free[%lu] \n", cnt, (unsigned int)ptr->is_used, ptr->size,
        (unsigned long)ptr->prev_block, (unsigned long)ptr->next_block, (unsigned long)ptr->prev_free_block,
        (unsigned long)ptr->next_free_block);
    ptr = ptr->next_free_block;
    ++cnt;
  }
  printf("Total free blocks: %d\n\n", cnt);
}
