#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include "goatmalloc.h"

#define PRINTF_GREEN(...) fprintf(stderr, "\033[32m"); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\033[0m");

void print_header(node_t *header){
  //Note: These printf statements may produce a segmentation fault if the buff
  //pointer is incorrect, e.g., if buff points to the start of the arena.
  printf("Header->size: %lu\n", header->size);
  printf("Header->fwd: %p\n", header->fwd);
  printf("Header->bwd: %p\n", header->bwd);
  printf("Header->is_free: %d\n", header->is_free);
}

void test_realloc_edge_cases(){
    int test=1;
    int page_size = getpagesize();
    void *buff, *buff2;
    node_t *header, *header2;
    
    PRINTF_GREEN(">>Testing wrealloc for edge cases.\n");
    init(page_size);

    // essentially walloc(0)
    buff = wrealloc(NULL, 0);
    assert(buff != NULL);
    PRINTF_GREEN("Assert %d passed!\n", test++);

    header = ((void*) buff) - sizeof(node_t);    
    print_header(header);
    assert(header->size == 0); 
    assert(header->bwd == NULL);
    assert(header->is_free == 0);
    assert(header->fwd == ((void*) header + sizeof(node_t) + 0));

    // test for splitting 
    node_t *next = header->fwd;
    print_header(next);
    
    assert(next->fwd == NULL);
    PRINTF_GREEN("Assert %d passed!\n", test++);
    assert(next->bwd == header);
    PRINTF_GREEN("Assert %d passed!\n", test++);
    assert(next->size == page_size-0-(sizeof(node_t)*2));
    PRINTF_GREEN("Assert %d passed!\n", test++);
    assert(next->is_free == 1);
    PRINTF_GREEN("Assert %d passed!\n", test++);

    // the areana now contains two chunks 
    // essentially wfree(buff) then walloc(0)
    buff2 = wrealloc(buff, 0);
    header2 = ((void*) buff2) - sizeof(node_t); 
    print_header(header2);

    assert(header2->size == 0); 
    assert(header2->bwd == NULL);
    assert(header2->is_free == 0);
    assert(header2->fwd == ((void*) header2 + sizeof(node_t) + 0));

    next = header2->fwd;
    print_header(next);
    
    assert(next->fwd == NULL);
    PRINTF_GREEN("Assert %d passed!\n", test++);
    assert(next->bwd == header2);
    PRINTF_GREEN("Assert %d passed!\n", test++);
    assert(next->size == page_size-0-(sizeof(node_t)*2));
    PRINTF_GREEN("Assert %d passed!\n", test++);
    assert(next->is_free == 1);
    PRINTF_GREEN("Assert %d passed!\n", test++);

    destroy();
}

void test_realloc_next_chunk_case1(){
  int test = 1; 
  int size, size2;
  int page_size;
  void *buff, *buff2;
  node_t *header, *header2;  

  PRINTF_GREEN(">>Testing wrealloc to next chunk. Case 1\n");

  page_size = getpagesize();
  init(page_size);

  size = 1;
  buff = walloc(size);
  // calculate the size needed to grow buff to the entire size 
  size2 = page_size - sizeof(node_t);
  buff2 = wrealloc(buff, size2);

  assert(buff2 != NULL);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(buff2 == buff);
  PRINTF_GREEN("Assert %d passed!\n", test++);

  header = ((void*)buff) - sizeof(node_t);
  header2 = ((void*)buff2) - sizeof(node_t);
  assert(header == header2);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->is_free == 0);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  // PRINTF_GREEN("header->size %zu, expected: %d\n", header->size, size2);
  assert(header->size == size2);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->bwd == NULL);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->fwd == NULL);

  destroy();
}

void test_realloc_next_chunk_case2(){
  int test = 1; 
  int size, size2;
  int page_size;
  void *buff, *buff2;
  node_t *header, *header2;  

  PRINTF_GREEN(">>Testing wrealloc to next chunk. Case 2\n");

  page_size = getpagesize();
  init(page_size);

  size = 16;
  buff = walloc(size);
  
  header = ((void*)buff) - sizeof(node_t);
  node_t *next = header->fwd;
  print_header(next);

  // case 2: the requested size does not take the entire next chunk, 
  // but the remaing size is not sufficient to split a free chunk 
  size2 = page_size - 2*sizeof(node_t);
  buff2 = wrealloc(buff, size2);
  assert(buff2 != NULL);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(buff2 == buff);
  PRINTF_GREEN("Assert %d passed!\n", test++);

  header2 = ((void*)buff2) - sizeof(node_t);
  assert(header == header2);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->is_free == 0);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  // PRINTF_GREEN("header->size %zu, expected: %d\n", header->size, size2);
  assert(header->size != size2);
  assert(header->size == (size + next->size + sizeof(node_t)));
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->bwd == NULL);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->fwd == NULL);
  PRINTF_GREEN("Assert %d passed!\n", test++);

  destroy();
    

}

void test_realloc_next_chunk_case3(){
  int test = 1; 
  int size, size2, size3;
  int page_size;
  void *buff, *buff2, *buff3;
  node_t *header, *header2;  

  PRINTF_GREEN(">>Testing wrealloc to next chunk. Case 3\n");

  page_size = getpagesize();
  init(page_size);

  size = 64;
  buff = walloc(size);
  buff2 = walloc(size);
  walloc(size);
  
  header = ((void*)buff) - sizeof(node_t);
  node_t *next = header->fwd;
  assert(next == ((void*)buff2) - sizeof(node_t));
  size2 = next->size;
  assert(size2 == size);
  print_header(next);

  wfree(buff2);


  // case 3: grow a tiny amount to test splitting 
  size3 = header->size + 1;
  buff3 = wrealloc(buff, size3);

  assert(buff3 != NULL);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(buff3 == buff);
  PRINTF_GREEN("Assert %d passed!\n", test++);

  header2 = ((void*)buff3) - sizeof(node_t);
  assert(header == header2);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->is_free == 0);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->size == size3);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->bwd == NULL);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  PRINTF_GREEN("header-fwd>size %zu, expected: %d\n", header->fwd->size, size2-1);
  assert(header->fwd != NULL);
  assert(header->fwd->size == size2 - 1);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->fwd->is_free == 1); 
  assert(header->fwd->fwd->is_free == 0); 
  PRINTF_GREEN("Assert %d passed!\n", test++);

  destroy();
    

}

void test_realloc_with_malloc(){
  // the current chunk does not have an eligible right neighbor 
  // there is an eligible free chunk (first-fit)
  
  PRINTF_GREEN(">>Testing wrealloc with walloc.\n");

  int test = 1;
  int page_size; 
  int size, size2;
  void *buff, *buff2, *buff3;
  node_t *header, *header2, *header3, *header4;


  page_size = getpagesize();
  init(page_size);

  size = 9;
  buff = walloc(size);
  buff2 = walloc(size);

  // write some unique chars to the allocated memory 
  char* ptr = buff;
  char content[] = {0xd, 0xe, 0xa, 0xd, 0xb, 0xe, 0xa, 0xf, '\0'};
  int i = 0;
  while (content[i] != '\0'){
    *ptr = content[i];
    i++;
    ptr++;
  }

  header = ((void*)buff) - sizeof(node_t);
  header2 = ((void*)buff2) - sizeof(node_t);
  
  // grow buff to be reasonable size 
  size2 = 64;
  buff3 = wrealloc(buff, size2);


  header3 = ((void*)buff3) - sizeof(node_t);
  header4 = (node_t *) ((void*)buff3 + size2);

  // buff should be freed, and the new chunk header at header2 
  assert(header->is_free == 1);
  assert(header3->is_free == 0);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  // the freed chunk does not trigger coalescing 
  assert(header->bwd == NULL);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->fwd == header2); 
  // the new chunk should trigger splitting 
  assert(header3->size == size2);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header3->bwd == header2);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header3->fwd != NULL); // splitting 
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header3->fwd == header4);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header4->is_free == 1);
  assert(header4->fwd == NULL);
  // the last chunk size 
  assert(header4->size == page_size - 2*size - size2 - 4*sizeof(node_t));
  PRINTF_GREEN("Assert %d passed!\n", test++);

  // the original data should have been copied over 
  ptr = buff3; 
  i = 0;
  while (content[i] != '\0'){
    assert(*ptr == content[i]);
    i++;
    ptr++;
  }

  PRINTF_GREEN("Assert %d passed!\n", test++);

  destroy();
  
}

void test_realloc_fail(){
  // when realloc fails, a NULL pointer is returned 
  // the original chunk is left untouched 
  // case 1: the current chunk does not have an eligible right neighbor 
  //  - right neigbor == NULL
  //  - right neighbor is not free 
  //  - right neighbor is too small 
  // and there is no eligible free chunks in the arena

  int test = 1; 
  int size, size2;
  int page_size;
  void *buff, *buff2;
  node_t *header;  

  PRINTF_GREEN(">>Testing realloc fail case.\n");

  page_size = getpagesize();
  init(page_size);

  size = page_size - sizeof(node_t);
  buff = walloc(size);
  char* ptr = buff;
  
  // write some unique chars to the allocated memory 
  char content[] = {0xd, 0xe, 0xa, 0xd, 0xb, 0xe, 0xa, 0xf, '\0'};
  int i = 0;
  while (content[i] != '\0'){
    *ptr = content[i];
    i++;
    ptr++;
  }
  
  // calculate the size needed to grow buff to the entire size 
  size2 = size + 1 ;
  buff2 = wrealloc(buff, size2);

  assert(buff2 == NULL);
  PRINTF_GREEN("Assert %d passed!\n", test++);

  header = ((void*)buff) - sizeof(node_t);
  assert(header->is_free == 0);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->size == size);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->bwd == NULL);
  PRINTF_GREEN("Assert %d passed!\n", test++);
  assert(header->fwd == NULL);
  PRINTF_GREEN("Assert %d passed!\n", test++);

  ptr = buff; 
  i = 0;
  while (content[i] != '\0'){
    assert(*ptr == content[i]);
    i++;
    ptr++;
  }
  PRINTF_GREEN("Assert %d passed!\n", test++);

  assert(statusno == ERR_OUT_OF_MEMORY);

  destroy();
    
}

int main(int argc, char **argv) {
  int test = atoi(argv[1]);

  switch(test) {
    case 1:
      test_realloc_edge_cases();
      break;
    case 2: 
      test_realloc_next_chunk_case1();
      break;
    case 3:
      test_realloc_next_chunk_case2();
      break;
    case 4:
      test_realloc_next_chunk_case3();
      break;
    case 5: 
      test_realloc_with_malloc();
      break;
    case 6:
      test_realloc_fail();
      break;
    default:
      break;
  }

}
