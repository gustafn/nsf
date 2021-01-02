/*
 * nsfUtil.c --
 *
 *      Utility functions of the Next Scripting Framework.
 *
 * Copyright (C) 2001-2017 Gustaf Neumann
 * Copyright (C) 2001-2007 Uwe Zdun
 *
 * Vienna University of Economics and Business
 * Institute of Information Systems and New Media
 * A-1020, Welthandelsplatz 1
 * Vienna, Austria
 *
 * This work is licensed under the MIT License https://www.opensource.org/licenses/MIT
 *
 * Copyright:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "nsfInt.h"

/*
 *----------------------------------------------------------------------
 * strnstr --
 *
 *      Implementation of strnstr() for platforms not providing it via their C
 *      library. The function strnstr locates the first occurrence of a
 *      substring in a null-terminated string.
 *
 * Results:
 *      Strbstring or NULL
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
#ifndef HAVE_STRNSTR
char *
strnstr(const char *buffer, const char *needle, size_t buffer_len) {
  char *result = NULL;

  if (*needle == '\0') {
    result = (char *)buffer;

  } else {
    size_t remainder, needle_len;
    char  *p;

    needle_len = strlen(needle);
    for (p = (char *)buffer, remainder = buffer_len;
         p != NULL;
         p = memchr(p + 1, *needle, remainder-1)) {
      remainder = buffer_len - (size_t)(p - buffer);
      if (remainder < needle_len) {
        break;
      }
      if (*p == *needle && strncmp(p, needle, needle_len) == 0) {
        result = p;
        break;
      }
    }
  }

  return result;
}
#endif

/*
 *----------------------------------------------------------------------
 * Nsf_ltoa --
 *
 *      Convert a long value into a string; this function is a fast
 *      version of sprintf(buf, "%ld", l);
 *
 * Results:
 *      String containing decimal value of the provided parameter.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
char *
Nsf_ltoa(char *buf, long i, int *lengthPtr) {
  int  nr_written, negative;
  char tmp[LONG_AS_STRING], *pointer = &tmp[1], *string, *p;

  nonnull_assert(buf != NULL);
  nonnull_assert(lengthPtr != NULL);

  tmp[0] = 0;

  if (i < 0) {
    i = -i;
    negative = nr_written = 1;
  } else {
    nr_written = negative = 0;
  }
  /*
   * Convert the binary value to a digit string in reversed order to the tmp
   * buffer since we do not know the length.
   */
  do {
    nr_written++;
    *pointer++ = (char)((i % 10) + '0');
    i /= 10;
  } while (i);

  p = string = buf;
  if (negative != 0) {
    *p++ = '-';
  }
  /*
   * Copy number (reversed) from tmp to buf.
   */
  while ((*p++ = *--pointer)) {
    ;
  }
  *lengthPtr = nr_written;

  return string;
}

/*
 *----------------------------------------------------------------------
 * NsfStringIncr --
 *
 *      Increment a value on a number system with the provided alphabet. The
 *      intention of the function is to generate compact new symbols.
 *
 * Results:
 *      New symbol in form of a string.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static const char *alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static size_t blockIncrement = 8u;
/*
  static char *alphabet = "ab";
  static int blockIncrement = 2;
*/
static unsigned char chartable[255] = {0};

char *
NsfStringIncr(NsfStringIncrStruct *iss) {
  char newch, *currentChar;

  nonnull_assert(iss != NULL);

  currentChar = iss->buffer + iss->bufSize - 2;
  newch = *(alphabet + chartable[(unsigned)*currentChar]);

  while (1) {
    if (newch != '\0') { /* no overflow */
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
          char  *newBuffer = ckalloc((unsigned)newBufSize);

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
 *      Support function for NsfStringIncr(). NsfStringIncrInit() function is
 *      called before NsfStringIncr() can be used on this buffer,
 *      NsfStringIncrFree() terminates usage.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Initializes the .
 *
 *----------------------------------------------------------------------
 */

void
NsfStringIncrInit(NsfStringIncrStruct *iss) {
  const char  *p;
  int          i = 0;
  const size_t bufSize = blockIncrement;
  /*
   * The static variable blockIncrement is always large than 2, otherwise we
   * would have to use:
   *
   *   const size_t bufSize = (blockIncrement > 2) ? blockIncrement : 2;
   */

  nonnull_assert(iss != NULL);

  for (p=alphabet; *p != '\0'; p++) {
    chartable[(int)*p] = (unsigned char)(++i);
  }

  iss->buffer = ckalloc((unsigned)bufSize);
  memset(iss->buffer, 0, bufSize);
  iss->start    = iss->buffer + bufSize-2;
  iss->bufSize  = bufSize;
  iss->length   = 1;
  /*
    for (i = 1; i < 50; i++) {
    NsfStringIncr(iss);
    fprintf(stderr, "string '%s' (%d)\n",  iss->start, iss->length);
    }
  */
}

void
NsfStringIncrFree(NsfStringIncrStruct *iss) {

  nonnull_assert(iss != NULL);

  ckfree(iss->buffer);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * indent-tabs-mode: nil
 * End:
 */
