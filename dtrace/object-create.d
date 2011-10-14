/* -*- D -*-
 *
 * check, if every object is freed.
 */

nsf*:::object-alloc { @[copyinstr(arg0)] = sum(1); }
nsf*:::object-free   { @[copyinstr(arg0)] = sum(-1); }

