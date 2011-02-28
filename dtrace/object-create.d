/* -*- D -*-
 *
 * check, if every object is freed.
 */

nsf*:::object-create { @[copyinstr(arg0)] = sum(1); }
nsf*:::object-free   { @[copyinstr(arg0)] = sum(-1); }

