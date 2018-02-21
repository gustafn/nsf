/* -*- D -*-
 *
 * nsf_calltime.d --
 *
 * Measure time between method-entry and method-returns
 *
 * Activate tracing between 
 *    ::nsf::configure dtrace on
 * and
 *    ::nsf::configure dtrace off
 *
 * FIELDS:
 *		PROVIDER	Object/class providing the method called.
 *		SCOPE		Scope for which the method called is provided (per-class, per-object, direct)
 *		NAME		Name of called method
 *		TOTAL		Total count of calls or elapsed time for calls (us)
 *
 * Copyright (c) 2018 Stefan Sobernig
 *
 * Vienna University of Economics and Business
 * Institute of Information Systems and New Media
 * A-1020, Welthandelsplatz 1
 * Vienna, Austria
 *
 * This work is licensed under the MIT License http://www.opensource.org/licenses/MIT
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
 * Based on https://github.com/opendtrace/toolkit/blob/master/Tcl/tcl_calltime.d
 *
 * COPYRIGHT: Copyright (c) 2007 Brendan Gregg.
 *
 * CDDL HEADER START
 *
 *  The contents of this file are subject to the terms of the
 *  Common Development and Distribution License, Version 1.0 only
 *  (the "License").  You may not use this file except in compliance
 *  with the License.
 *
 *  You can obtain a copy of the license at Docs/cddl1.txt
 *  or http://www.opensolaris.org/os/licensing.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 * CDDL HEADER END
 *
 * 09-Sep-2007	Brendan Gregg	Created this.
 */

dtrace:::BEGIN
{
	printf("Tracing... Hit Ctrl-C to end.\n");
	top = 20;
}

nsf*:::configure-probe /!self->tracing && copyinstr(arg0) == "dtrace" / {
  self->tracing = (arg1 && copyinstr(arg1) == "on") ? 1 : 0;
} 

nsf*:::configure-probe /self->tracing && copyinstr(arg0) == "dtrace" / {
  self->tracing = (arg1 && copyinstr(arg1) == "off") ? 0 : 1;
}

/*
 * Measure time differences
 */
nsf*:::method-entry /self->tracing/ {
        self->depth++;
	self->exclude[self->depth] = 0;
        self->proc[self->depth] = timestamp;
}

nsf*:::method-return /self->tracing && self->proc[self->depth] / {

        this->elapsed_incl = timestamp - self->proc[self->depth];
	this->elapsed_excl = this->elapsed_incl - self->exclude[self->depth];
	self->proc[self->depth] = 0;
	self->exclude[self->depth] = 0;
	this->name = copyinstr(arg2);
	this->scope = (copyinstr(arg1) != "") ? ((copyinstr(arg0) == copyinstr(arg1)) ? "object" : "class") : "direct";
	
	@num[copyinstr(arg1), this->scope, this->name] = count();
	@num["", "total", "-"] = count();
	@types_incl[copyinstr(arg0), this->scope, this->name] = sum(this->elapsed_incl);
	@types_excl[copyinstr(arg0), this->scope, this->name] = sum(this->elapsed_excl);
	@types_excl["", "total", "-"] = sum(this->elapsed_excl);

	self->depth--;
        self->exclude[self->depth] += this->elapsed_incl;
}

/*
 * Print aggregate with own format (less wide than the default)
 */
END {
    trunc(@num, top);
	printf("\nTop %d counts,\n", top);
	printf("   %48s %-10s %-48s %8s\n", "PROVIDER", "SCOPE", "NAME", "COUNT");
	printa("   %48s %-10s %-48s %@8d\n", @num);

     trunc(@types_excl, top);
	normalize(@types_excl, 1000);
	printf("\nTop %d exclusive elapsed times (us),\n", top);
	printf("   %48s %-10s %-48s %8s\n", "PROVIDER", "SCOPE", "NAME", "TOTAL");
	printa("   %48s %-10s %-48s %@8d\n", @types_excl);

	trunc(@types_incl, top);
	normalize(@types_incl, 1000);
	printf("\nTop %d inclusive elapsed times (us),\n", top);
	printf("   %48s %-10s %-48s %8s\n", "PROVIDER", "SCOPE", "NAME", "TOTAL");
        printa("   %48s %-10s %-48s %@8d\n", @types_incl);
}
