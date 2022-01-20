#include "my_malloc.h"

//meta_data_t * block_head = NULL;
//meta_data_t * block_tail = NULL;
meta_data_t * free_list_head = NULL;
meta_data_t * free_list_tail = NULL;
unsigned long segment_size = 0;
unsigned long segment_free_space_size = 0;

void *ff_malloc(size_t size) {
  return  f_malloc(size, &find_existed_block_ff);
}

void ff_free(void *ptr) {
  f_free(ptr);
}

void *bf_malloc(size_t size) {
  return  f_malloc(size, &find_existed_block_bf);
}

void bf_free(void *ptr) {
  f_free(ptr);
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
    void * ptr = add_new_block(size);
    if (ptr == (void *) -1) {
      return NULL;
    } else {
      return ptr + sizeof(meta_data_t);
    }
  }
  assert(((meta_data_t *)addr)->is_used == 1);
  return addr + sizeof(meta_data_t);
}

void * try_existed_block(size_t size, add_func_t f) {
  meta_data_t * ptr = f(size);
  // no available free block
  if (ptr == NULL) {
    return NULL;
  }
  assert(ptr->is_used == 0);
  // need to seperate into two blocks
  if (ptr->size > size + sizeof(meta_data_t)) {
    split_block(ptr, size);
  } else {
    remove_block(ptr);
  }
  return ptr;
}

meta_data_t * find_existed_block_ff(size_t size) {
  meta_data_t * ptr = free_list_head;
  while (ptr != NULL) {
    assert(ptr->is_used == 0);
    if (ptr->size >= size) {
      return ptr;
    }
    ptr = ptr->next_free_block;
  }
  return NULL;
}

meta_data_t * find_existed_block_bf(size_t size) {
  meta_data_t * ptr = free_list_head;
  size_t min = SIZE_MAX;
  meta_data_t * min_ptr = NULL;
  while (ptr != NULL) {
    if (ptr->is_used != 0) {
      print_block(ptr);
    }
    assert(ptr->is_used == 0);
    if(ptr->size == size) {
      return ptr;
    } else if (ptr->size > size && ptr->size < min) {
      min = ptr->size;
      min_ptr = ptr;
    }
    ptr = ptr->next_free_block;
  }
  return min_ptr;
}

void * add_new_block(size_t size) {
  void * ptr = sbrk(size + sizeof(meta_data_t));
  if (ptr == (void * )-1) {
    return ptr;
  }
  meta_data_t * block_meta = (meta_data_t *) ptr;
  block_meta->size = size;
  block_meta->is_used = 1;
  block_meta->prev_free_block = NULL;
  block_meta->next_free_block = NULL;
  
  segment_size += (size + sizeof(meta_data_t));
  return ptr;
}

void f_free(void * ptr) {
  if (ptr == NULL) {
    return;
  }
  assert ((unsigned long)ptr >= sizeof(meta_data_t));
  meta_data_t * block = (meta_data_t *) (ptr - sizeof(meta_data_t));
  assert(block->is_used == 1);
  add_to_free_list(block); 
  try_coalesce(block);
}

void try_coalesce(meta_data_t * block) {
  assert(block->is_used == 0);
  if (block->next_free_block != NULL &&
    (void *)(block) + sizeof(meta_data_t) + block->size == block->next_free_block) {
    assert(block->next_free_block->is_used == 0);
    block->size += (block->next_free_block->size + sizeof(meta_data_t));
    if (block->next_free_block->next_free_block != NULL) {
      block->next_free_block->next_free_block->prev_free_block = block;
    } else {
      free_list_tail = block;
    }
    block->next_free_block = block->next_free_block->next_free_block; 
  }
  if(block->prev_free_block != NULL &&
    (void *)(block->prev_free_block) + sizeof(meta_data_t) + block->prev_free_block->size == block) {
    assert(block->prev_free_block->is_used == 0);
    block->prev_free_block->size += (block->size + sizeof(meta_data_t));
    if(block->next_free_block != NULL) {
      block->next_free_block->prev_free_block = block->prev_free_block;
    } else {
      free_list_tail = block->prev_free_block;
    }
    block->prev_free_block->next_free_block = block->next_free_block;
  }
}

void add_to_free_list(meta_data_t * block) {
  assert(block->is_used == 1);
  assert(block->prev_free_block == NULL && block->next_free_block == NULL);
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

void remove_block(meta_data_t * block) {
  assert(block->is_used == 0);
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

// split an unused block into two, use the first one
void split_block(meta_data_t * block1, size_t size) {
  // important assertion
  assert(block1->is_used == 0);
  assert(block1->size - size > sizeof(meta_data_t));
  meta_data_t * block2 = (meta_data_t *)((void *)block1 + size + sizeof(meta_data_t));
  block2->prev_free_block = block1->prev_free_block;
  block2->next_free_block = block1->next_free_block;
  if (block2->next_free_block != NULL) {
    block2->next_free_block->prev_free_block = block2;
  } else {
    free_list_tail = block2;
  }
  if (block2->prev_free_block != NULL) {
    block2->prev_free_block->next_free_block = block2;
  } else {
    free_list_head = block2;
  }
  block1->is_used = 1;
  block2->is_used = 0;
  block2->size = block1->size - sizeof(meta_data_t) - size;
  block1->size = size;
  block1->prev_free_block = NULL;
  block1->next_free_block = NULL;
  segment_free_space_size -= (size + sizeof(meta_data_t));
}

/*
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
*/
void print_free_list() {
  printf("***Free List Data***\n");
  int cnt = 0;
  meta_data_t * ptr = free_list_head;
  while (ptr != NULL) {
    printf("free block %d [%lu]: is_used[%u], size[%lu], prev_free[%lu], next_free[%lu] \n", 
        cnt, (unsigned long) ptr, (unsigned int)ptr->is_used, ptr->size, (unsigned long)ptr->prev_free_block,
        (unsigned long)ptr->next_free_block);
    ptr = ptr->next_free_block;
    ++cnt;
  }
  printf("Total free blocks: %d\n\n", cnt);
}

void print_block(meta_data_t * ptr) {
  printf("***Block Data***\n");
  printf("free block[%lu]: is_used[%u], size[%lu], prev_free[%lu], next_free[%lu] \n", 
      (unsigned long)ptr, (unsigned int)ptr->is_used, ptr->size, (unsigned long)ptr->prev_free_block,
      (unsigned long)ptr->next_free_block);
}

void print_sizeof_metadata() {
  printf("sizeof meta_data_t is %ld\n", sizeof(meta_data_t));
}
