#include "my_malloc.h"

void test_add_new_block() {
  void * ptr = add_new_block(16);
  printf("ptr for add_new_block = %p\n", ptr);
}

int main() {
  test_add_new_block(); 
}
