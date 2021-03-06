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
 * $Id: child.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_CHILD_H
#define LIB_CHILD_H

/* ------------------------------------------------------------------------ *
 * System headers                                                             *
 * ------------------------------------------------------------------------ */
#include <sys/types.h>

/* ------------------------------------------------------------------------ *
 * Library headers                                                            *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/dlink.h"
#include "libchaos/timer.h"

/* ------------------------------------------------------------------------ *
 * Constants                                                                  *
 * ------------------------------------------------------------------------ */

#define CHILD_IDLE       0x00          /* child block just sitting there */
#define CHILD_RUNNING    0x01
#define CHILD_DEAD       0x02

#define CHILD_MAX_CHANNEL 4      /* max. I/O channels to the child */

#define CHILD_READ   0
#define CHILD_WRITE  1

#define CHILD_PARENT 0
#define CHILD_CHILD  1

#define CHILD_INTERVAL   5000L

/* ------------------------------------------------------------------------ *
 * Types                                                                      *
 * ------------------------------------------------------------------------ */
struct child;
typedef void (child_cb_t)(struct child *, void *, void *, void *, void *);

/* ------------------------------------------------------------------------ *
 * Child block structure.                                                     *
 * ------------------------------------------------------------------------ */
struct child
{
  struct node        node;         /* linking node for child block list */

  /* externally initialised */
  long               pid;
  uint32_t           id;
  uint32_t           refcount;
  hash_t             chash;
  hash_t             nhash;
  int                status;
  struct timer      *timer;
  uint32_t           chans;
  int                channels[CHILD_MAX_CHANNEL][2][2];
  int                autostart;
  int                exitcode;
  uint64_t           interval;
  uint64_t           start;        /* time at which the child was started */
  char               name[HOSTLEN];
  char               path[PATHLEN];
  char               argv[64];
  char               arguments[CHILD_MAX_CHANNEL * 4][16];
  void              *args[4];
  child_cb_t        *callback;
};

/* ------------------------------------------------------------------------ *
 * Global variables                                                           *
 * ------------------------------------------------------------------------ */
CHAOS_DATA(int)            child_log;          /* Log source */
CHAOS_DATA(struct sheap)  child_heap;         /* Heap containing child blocks */
CHAOS_DATA(struct list)   child_list;         /* List linking child blocks */
CHAOS_DATA(int)           child_dirty;        /* we need a garbage collect */
CHAOS_DATA(uint32_t)      child_id;           /* Next serial number */

/* ------------------------------------------------------------------------ */
CHAOS_API(int  child_get_log(void))

/* ------------------------------------------------------------------------ *
 * Initialize child heap and add garbage collect timer.                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           child_init         (void))

/* ------------------------------------------------------------------------ *
 * Destroy child heap and cancel timer.                                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           child_shutdown     (void))

/* ------------------------------------------------------------------------ *
 * Garbage collect child blocks                                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           child_collect      (void))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           child_default      (struct child  *cdptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct child * child_new          (const char    *path,
                                             uint32_t       channels,
                                             const char    *argv,
                                             uint64_t       interval,
                                             int            autostart))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int            child_update       (struct child  *cdptr,
                                             uint32_t       channels,
                                             const char    *argv,
                                             uint64_t       interval,
                                             int            autostart))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           child_delete       (struct child  *cdptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct child * child_find         (const char    *path))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct child * child_pop          (struct child  *cdptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct child * child_push         (struct child **cdptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int            child_launch       (struct child  *cdptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           child_cancel       (struct child  *cdptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           child_kill         (struct child  *cdptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           child_set_callback (struct child  *cdptr,
                                             void          *callback))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           child_vset_args    (struct child  *cdptr,
                                             va_list        args))

CHAOS_API(void           child_set_args     (struct child  *cdptr,
                                             ...))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           child_set_name     (struct child  *cdptr,
                                         const char    *name))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(const char *   child_get_name     (struct child  *cdptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct child * child_find_name    (const char    *name))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct child * child_find_id      (uint32_t       id))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           child_dump         (struct child  *cdptr))

#endif
