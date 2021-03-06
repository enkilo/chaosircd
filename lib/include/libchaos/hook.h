/* chaosircd - Chaoz's IRC daemon daemon
 *
 * Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 *
 * $Id: hook.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_HOOK_H
#define LIB_HOOK_H

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#include "libchaos/dlink.h"

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#define HOOK_1ST 0x00
#define HOOK_2ND 0x01
#define HOOK_3RD 0x02
#define HOOK_4TH 0x03
#define HOOK_DEFAULT HOOK_1ST

#define HOOK_MAXARGS 4

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
typedef int (hook_cb_t)(void *arg0, void *arg1, void *arg2, void *arg3);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct hook {
  struct node node;
  void       *function;
  int         type;
  hook_cb_t  *callback;
};

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void          hook_init       (void))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void          hook_shutdown   (void))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct hook * hook_register   (void *function,
                                         int   type,
                                         void *callback))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int           hook_unregister (void *function,
                                         int   type,
                                         void *callback))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int           hooks_call      (void *function,
                                         int   type,
                                         ...))

#endif
