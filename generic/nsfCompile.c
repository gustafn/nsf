/*
 * nsfCompile.c --
 *
 *      Support for Bytecode in XOTcl/Nsf. This code was written for Tcl 8.4
 *      and required a small change for Tcl 8.4 (available from www.xotcl.org,
 *      but not part of the Tcl Toolkit). This file was adopted only slightly
 *      for naming changes etc., but requires more work to be revived. The
 *      code is currently deactivated.
 *
 * Copyright (C) 2005-2015 Gustaf Neumann
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
 */

#include "nsfInt.h"

#ifdef NSF_BYTECODE
#include <tclCompile.h>

static CompileProc
  initProcNsCompile, nextCompile,
  selfCompile, selfDispatchCompile;

static InstructionDesc instructionTable[] = {
  {"initProc",		  1,   0,   {OPERAND_NONE}},
  {"next",		  1,   0,   {OPERAND_NONE}},
  {"self",		  1,   0,   {OPERAND_NONE}},
  {"dispatch",		  2,   1,   {OPERAND_UINT1}},
};

static NsfCompEnv instructions[] = {
  {0, 0, initProcNsCompile, NsfInitProcNSCmd},
  {0, 0, nextCompile, NsfNextObjCmd},
  {0, 0, selfCompile, NsfGetSelfObjCmd},
  {0, 0, selfDispatchCompile, /*NsfSelfDispatchCmd*/NsfDirectSelfDispatch},
  0
};

NsfCompEnv *
NsfGetCompEnv(void) {
  return &instructions[0];
}


static int initProcNsCompile(Tcl_Interp *interp, Tcl_Parse *parsePtr,
		  CompileEnv *envPtr) nonnull(1) nonnull(2) nonnull(3);

static int
initProcNsCompile(Tcl_Interp *interp, Tcl_Parse *parsePtr,
		  CompileEnv *envPtr) {

  assert(interp != NULL);
  assert(parsePtr != NULL);
  assert(envPtr != NULL);

  if (parsePtr->numWords != 1) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
	        "wrong # args: should be '::nsf::initProcNS'", -1);
	envPtr->maxStackDepth = 0;
	return TCL_ERROR;
    }

  TclEmitOpcode(instructions[INST_INITPROC].bytecode, envPtr);
  envPtr->maxStackDepth = 0;

  return TCL_OK;
}

static int nextCompile(Tcl_Interp *interp, Tcl_Parse *parsePtr,
		  CompileEnv *envPtr) nonnull(1) nonnull(2) nonnull(3);

static int
nextCompile(Tcl_Interp *interp, Tcl_Parse *parsePtr,
		  CompileEnv *envPtr) {

  assert(interp != NULL);
  assert(parsePtr != NULL);
  assert(envPtr != NULL);

  if (parsePtr->numWords != 1) {
    return TCL_OUT_LINE_COMPILE;
  }
  TclEmitOpcode(instructions[INST_NEXT].bytecode, envPtr);
  envPtr->maxStackDepth = 0;

  return TCL_OK;
}

static int selfCompile(Tcl_Interp *interp, Tcl_Parse *parsePtr,
		  CompileEnv *envPtr) nonnull(1) nonnull(2) nonnull(3);

static int
selfCompile(Tcl_Interp *interp, Tcl_Parse *parsePtr,
		  CompileEnv *envPtr) {

  assert(interp != NULL);
  assert(parsePtr != NULL);
  assert(envPtr != NULL);

  if (parsePtr->numWords != 1) {
    return TCL_OUT_LINE_COMPILE;
  }
  TclEmitOpcode(instructions[INST_SELF].bytecode, envPtr);
  envPtr->maxStackDepth = 0;

  return TCL_OK;
}

static int selfDispatchCompile(Tcl_Interp *interp, Tcl_Parse *parsePtr,
		  CompileEnv *envPtr) nonnull(1) nonnull(2) nonnull(3);

static int
selfDispatchCompile(Tcl_Interp *interp, Tcl_Parse *parsePtr,
		  CompileEnv *envPtr) {

  Tcl_Token *tokenPtr;
  int code, wordIdx;

  assert(interp != NULL);
  assert(parsePtr != NULL);
  assert(envPtr != NULL);

  /*
  fprintf(stderr, "****** selfDispatchCompile words=%d tokens=%d, avail=%d\n",
	  parsePtr->numWords, parsePtr->numTokens, parsePtr->tokensAvailable);
  */

  if (parsePtr->numWords > 255) {
    return TCL_OUT_LINE_COMPILE;
  }
  /*TclEmitOpcode(instructions[INST_SELF].bytecode, envPtr);*/

  for (wordIdx=0, tokenPtr = parsePtr->tokenPtr + 0;
       wordIdx < parsePtr->numWords;
       wordIdx++, tokenPtr += (tokenPtr->numComponents + 1)) {

    /*
    fprintf(stderr, "  %d: %p token type=%d size=%d\n",
	    wordIdx, tokenPtr, tokenPtr->type, tokenPtr->size );
    */
    if (tokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
      TclEmitPush(TclRegisterLiteral(envPtr, tokenPtr->start,
				     tokenPtr->size, 0), envPtr);
      envPtr->maxStackDepth = 1;
      /*
      fprintf(stderr, "  %d: simple '%s' components=%d\n",
	      wordIdx, tokenPtr->start, tokenPtr->numComponents);
      */
    } else {
      /*
      fprintf(stderr, "  %d NOT simple '%s' components=%d\n",
	      wordIdx, tokenPtr->start, tokenPtr->numComponents);
      */
      code = TclCompileTokens(interp, tokenPtr+1,
			      tokenPtr->numComponents, envPtr);
      if (code != TCL_OK) {
	return code;
      }
    }
  }

  /*fprintf(stderr, "maxdepth=%d, onStack=%d\n", envPtr->maxStackDepth, wordIdx);
   */
  TclEmitInstInt1(instructions[INST_SELF_DISPATCH].bytecode, wordIdx, envPtr);
  envPtr->maxStackDepth = 0;

  return TCL_OK;
}



void
NsfBytecodeInit(void) {
  int i;

  for(i=0; i<LAST_INSTRUCTION; i++) {
    if ((instructions[i].bytecode =
       TclRegisterUserOpcode(&instructionTable[i],
			     instructions[i].callProc,
			     instructions[i].cmdPtr->objClientData))) {
      instructions[i].cmdPtr->compileProc = instructions[i].compileProc;
    }

  }
  /*tclTraceCompile = 2;*/

}

#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * indent-tabs-mode: nil
 * End:
 */
