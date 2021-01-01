/* 
 *      threaded.c --
 *
 *      A small study on implementation techniques for threaded code.
 *      
 *      As example a small program stacking two values on to the stack
 *      and adding the result is taken (example from wikipedia). This
 *      program uses the abstract machine instructions PUSH_A, PUSH_B,
 *      ADD, and END. These instructions are coded in different ways:
 *      
 *      * switch_threading(): realizing instructions as C enum
 *        values, the program is represented as an array of enums,
 *        placed in a large surrounding switch statement.
 *
 *      * call_threading(): realizing instructions as C functions, the
 *        program is represented an array of C functions.
 *
 *      * label_threading(): realizing instructions as labels, the
 *        program is represented an array of labels.
 *
 *      The test show, that label_threading is clearly the winner in
 *      terms of performance, but this depends on a non standard C
 *      extension, which is implemented in at least three different
 *      compilers (supported on GCC, clang and IBM's XL C/C++). It
 *      would be interesting to define the instructions in some
 *      e.g. scripting language and to produce different
 *      implementations from the same source to address the
 *      portability issue.
 *
 *      Most probably, one needs a larger program-code with more
 *      instructions to provide more meaningful results.
 *
 *      Compile e.g. with:
 *           cc -O3 -Wall threaded.c   -o threaded
 *
 *      Gustaf Neumann (Nov 2011)
 */

#include <stdio.h>
#include <sys/time.h>

/*
 *----------------------------------------------------------------------
 * DiffTime --
 *
 *      Compute the time difference between two timevals.
 *
 *----------------------------------------------------------------------
 */

static void
DiffTime(struct timeval *t1, struct timeval *t0, struct timeval *diffPtr) {
  diffPtr->tv_sec = t1->tv_sec - t0->tv_sec;
  diffPtr->tv_usec = t1->tv_usec - t0->tv_usec;
  if (diffPtr->tv_usec < 0) {
    diffPtr->tv_sec += (diffPtr->tv_usec / 1000000L) - 1;
    diffPtr->tv_usec = (diffPtr->tv_usec % 1000000L) + 1000000L;
  } else if (diffPtr->tv_usec > 1000000L) {
    diffPtr->tv_sec += diffPtr->tv_usec / 1000000L;
    diffPtr->tv_usec = diffPtr->tv_usec % 1000000L;
  }
}

/*
 *----------------------------------------------------------------------
 * timeit --
 *
 *      Run a function multiple times and measure the performance.
 *
 *----------------------------------------------------------------------
 */

static void timeit( void (* fn)() ) {
  struct timeval start, end, diff;
  int i;

  gettimeofday(&start, NULL);

  for (i=0; i<10000000; i++) {
    (*fn)();
  }

  gettimeofday(&end, NULL);
  DiffTime(&end, &start, &diff);
  printf("%d seconds, %d usec\n", (int) diff.tv_sec, (int) diff.tv_usec);
}


int *sp;
int stack[100];

/*
 *----------------------------------------------------------------------
 * switch_threading --
 *
 *      Define an enum for the instructions and use a switch to select
 *      the instructions.
 *
 *----------------------------------------------------------------------
 */
void switch_threading() {
  typedef enum {INST_PUSH_A, INST_PUSH_B, INST_ADD, INST_END} InstEnum;
  static InstEnum mycode[] = {INST_PUSH_A, INST_PUSH_B, INST_ADD, INST_END};
  int a, b;
  InstEnum *ip;

  ip = &mycode[0];
  sp = &stack[0];

  for (;;) {
    switch (*ip++) {
    case INST_PUSH_A:
      *sp++ = 100;
      continue;
    case INST_PUSH_B:
      *sp++ = 200;
      continue;      
    case INST_ADD: 
      a = *--sp;
      b = *--sp;
      *sp++ = a + b;
      continue;
    case INST_END: 
      //fprintf(stderr, "end %d\n", *(sp-1));
      return;
    }
  }
}

/*
 *----------------------------------------------------------------------
 * call_threading --
 *
 *      Define for every instruction a function, the program consists of
 *      an array of function pointers.
 *
 *----------------------------------------------------------------------
 */
typedef void (* InstFn)();

void pushA () { *sp++ = 100; }
void pushB () { *sp++ = 200; }
void add   () { int a = *--sp; int b = *--sp; *sp++ = a + b; }

void call_threading() {
  static InstFn mycode[] = {&pushA, &pushB, &add, NULL};
  InstFn *ip;

  sp = &stack[0];
  ip = &mycode[0];

  while (*ip) {
    (*ip++)();
  }

  //fprintf(stderr, "end %d\n", *(sp-1));
}

/*
 *----------------------------------------------------------------------
 * label_threading --
 *
 *      Define for every instruction a label, the code is a sequence of
 *      labels. This works with gcc, clang and IBM's XL C, but not on
 *      every compiler.
 *
 *----------------------------------------------------------------------
 */
typedef void (* InstLabel)();

void label_threading() {
  static InstLabel mycode[] = {&&INST_PUSH_A, &&INST_PUSH_B, &&INST_ADD, &&INST_END};
  InstLabel *ip;
  int a, b;

  sp = &stack[0];
  ip = &mycode[0];

 INST_PUSH_A:
  *sp++ = 100;
  goto **ip++;

 INST_PUSH_B:
  *sp++ = 200;
  goto **ip++;

 INST_ADD: 
  a = *--sp;
  b = *--sp;
  *sp++ = a + b;  
  goto **ip++;
  
 INST_END: 
  //fprintf(stderr, "end %d\n", *(sp-1));
  return;
}

/*
 *----------------------------------------------------------------------
 * main --
 *
 *      Just call the testcases with timing.
 *
 *----------------------------------------------------------------------
 */

int main() {

  timeit( switch_threading );
  timeit( call_threading   );
  timeit( label_threading  );

  return 0;
}
