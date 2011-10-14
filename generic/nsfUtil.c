/* 
 *  
 *  Next Scripting Framework
 *
 *  Copyright (C) 1999-2010 Gustaf Neumann, Uwe Zdun
 *
 *
 *  nsfUtil.c --
 *  
 *  Utility functions
 *  
 */

#include "nsfInt.h"

/*
 *----------------------------------------------------------------------
 * strnstr --
 *
 *    Implementation of strnstr() for platforms not providing it via their C
 *    library. The function strnstr locates the first occurance of a substring
 *    in a null-terminated string
 *
 * Results:
 *    Strbstring or NULL
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
#ifndef HAVE_STRNSTR
char *strnstr(const char *buffer, const char *needle, size_t buffer_len) {
  char *p;
  size_t remainder, needle_len;

  if (*needle == '\0') {
    return (char *)buffer;
  }

  needle_len = strlen(needle);
  for (p = (char *)buffer, remainder = buffer_len; 
       p != NULL; 
       p = memchr(p + 1, *needle, remainder-1)) {
    remainder = buffer_len - (p - buffer);
    if (remainder < needle_len) break;
    if (strncmp(p, needle, needle_len) == 0) {
      return p;
    }
  }

  return NULL;
}
#endif

/*
 *----------------------------------------------------------------------
 * Nsf_ltoa --
 *
 *    Convert a long value into a string; this function is a fast 
 *    version of sprintf(buf, "%ld", l);
 *
 * Results:
 *    String containing decimal value of the provided parameter.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
char *
Nsf_ltoa(char *buf, long i, int *len) {
  int nr_written, negative;
  char tmp[LONG_AS_STRING], *pointer = &tmp[1], *string, *p;
  *tmp = 0;
  
  if (i<0) {
    i = -i;
    negative = nr_written = 1;
  } else 
    nr_written = negative = 0;
  
  do {
    nr_written++;
    *pointer++ = i%10 + '0';
    i/=10;
  } while (i);
  
  p = string = buf;
  if (negative)
    *p++ = '-';
  
  while ((*p++ = *--pointer));   /* copy number (reversed) from tmp to buf */
  if (len) *len = nr_written;
  return string;
}

/*
 *----------------------------------------------------------------------
 * NsfStringIncr --
 *
 *    Increment a value on a number system with the provided alphabet. The
 *    intention of the function is to generate compact new symbols.
 *
 * Results:
 *    New symbol in form of a string.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static char *alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static int blockIncrement = 8;
/*
static char *alphabet = "ab";
static int blockIncrement = 2;
*/
static unsigned char chartable[255] = {0};

char *
NsfStringIncr(NsfStringIncrStruct *iss) {
  char newch, *currentChar;

  currentChar = iss->buffer + iss->bufSize - 2;
  newch = *(alphabet + chartable[(unsigned)*currentChar]);
    
  while (1) {
    if (newch) { /* no overflow */
      *currentChar = newch;
      break;
    } else {     /* overflow */
      *currentChar = *alphabet; /* use first char from alphabet */
      currentChar--;
      assert(currentChar >= iss->buffer);

      newch = *(alphabet + chartable[(unsigned)*currentChar]);
      if (currentChar < iss->start) {
	iss->length++;
	if (currentChar == iss->buffer) {
	  size_t newBufSize = iss->bufSize + blockIncrement;
	  char *newBuffer = ckalloc(newBufSize);
	  currentChar = newBuffer+blockIncrement;
	  /*memset(newBuffer, 0, blockIncrement);*/
	  memcpy(currentChar, iss->buffer, iss->bufSize);
	  *currentChar = newch;
	  iss->start = currentChar;
	  ckfree(iss->buffer);
	  iss->buffer = newBuffer;
	  iss->bufSize = newBufSize;
	} else {
	  iss->start = currentChar;
	}
      }
    }
  }
  assert(iss->buffer[iss->bufSize-1] == 0);
  assert(iss->buffer[iss->bufSize-2] != 0);
  assert(iss->length < iss->bufSize);
  assert(iss->start + iss->length + 1 == iss->buffer + iss->bufSize);

  return iss->start;
}

/*
 *----------------------------------------------------------------------
 * NsfStringIncrInit, NsfStringIncrFree --
 *
 *    Support function for NsfStringIncr(). NsfStringIncrInit() function is
 *    called before NsfStringIncr() can be used on this buffer,
 *    NsfStringIncrFree() terminates usage.
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Initializes the .
 *
 *----------------------------------------------------------------------
 */

void
NsfStringIncrInit(NsfStringIncrStruct *iss) {
  char *p;
  int i = 0;
  const size_t bufSize = blockIncrement>2 ? blockIncrement : 2;

  for (p=alphabet; *p; p++) {
    chartable[(int)*p] = ++i;
  }

  iss->buffer = ckalloc(bufSize);
  memset(iss->buffer, 0, bufSize);
  iss->start    = iss->buffer + bufSize-2;
  iss->bufSize  = bufSize;
  iss->length   = 1;
  /*
    for (i=1; i<50; i++) {
      NsfStringIncr(iss);
      fprintf(stderr, "string '%s' (%d)\n",  iss->start, iss->length);
    }
  */
}

void
NsfStringIncrFree(NsfStringIncrStruct *iss) {
  ckfree(iss->buffer);
}

