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
 * $Id: module.c,v 1.4 2006/09/28 08:38:31 roman Exp $
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* ------------------------------------------------------------------------ *
 * Library headers                                                          *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/syscall.h"
#include "libchaos/module.h"
#include "libchaos/timer.h"
#include "libchaos/dlink.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/str.h"

#include "config.h"

#ifndef USE_DSO
#define USE_DSO
#endif

#ifdef DARWIN
#include "dlfcn_darwin.h"
#endif

#ifndef USE_DSO
#include <../modules/module_import.h>
#endif

/* ------------------------------------------------------------------------ *
 * System headers                                                           *
 * ------------------------------------------------------------------------ */
#ifdef HAVE_ELF_H
#include <elf.h>
#endif

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#else
#include "./dlfcn.h"
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int           module_log;
struct sheap  module_heap;
struct list   module_list;
struct timer *module_timer;
uint32_t      module_id;
const char   *module_path = PLUGINDIR;
/* DAMN this shouldn't be here :/ */
const char   *module_table[][2] = {
  { "m",  "msg"      },
  { "um", "usermode" },
  { "cm", "chanmode" },
  { "lc", "lclient"  },
  { "sv", "service"  },
  { "st", "stats"    },
  { NULL, NULL       }
};

int module_dlopen(struct module *mptr);

/* ------------------------------------------------------------------------ */
int module_get_log() { return module_log; }

/* ------------------------------------------------------------------------ *
 * Load a module.                                                           *
 * ------------------------------------------------------------------------ */
static struct module *module_new(const char *path)
{
  char           name[128];
  char          *p;
  void          *handle;
  int          (*load)(struct module *);
  void         (*unload)(struct module *);
  struct module *ret = NULL;
  char           fn[64];
#ifndef USE_DSO
  uint32_t       i;
#endif

  /* cut path in front of module.so */
  p = strrchr(path, '/');

  strlcpy(name, (p ? p + 1 : path), sizeof(name));

  /* cut the .so */
  p = strchr(name, '.');

  if(p)
    *p = '\0';

  /* now open the DSO */
#ifndef USE_DSO
  handle = NULL;

  load = NULL; unload = NULL;

  for(i = 0; i < sizeof(module_imports) / sizeof(module_imports[0]); i++)
  {
    if(!str_cmp(module_imports[i].name, name))
    {
      load = module_imports[i].load;
      unload = module_imports[i].unload;
    }
  }
#else
#ifdef WIN32

  /* FIXME: should convert forward slashes to backslashes first */
  handle = LoadLibrary(path);

#else

  handle = dlopen(path, RTLD_NOW);

#endif

  if(handle == NULL)
  {
#ifdef USE_DSO
    log(module_log, L_warning, "DL error in %s: %s", path, dlerror());
#endif
    return NULL;
  }

  str_snprintf(fn, sizeof(fn), "%s_load", name);
  load = dlsym(handle, fn);
  str_snprintf(fn, sizeof(fn), "%s_unload", name);
  unload = dlsym(handle, fn);

#endif
  if(load == NULL || unload == NULL)
  {
#ifdef USE_DSO
    log(module_log, L_warning, "DL error: %s", dlerror());
    dlclose(handle);
#endif
    return NULL;
  }

  ret = mem_static_alloc(&module_heap);

  strlcpy(ret->path, path, sizeof(ret->path));
  strlcpy(ret->name, name, sizeof(ret->name));

  ret->phash = str_hash(ret->path);
  ret->nhash = str_ihash(ret->name);

  ret->handle = handle;
  ret->load = load;
  ret->unload = unload;
  ret->id = module_id++;
  ret->refcount = 1;

  if(ret->load(ret))
  {
    log(module_log, L_warning, "module load() failed.");

#ifdef USE_DSO
    dlclose(handle);
#endif
    mem_static_free(&module_heap, ret);
    return NULL;
  }
  return ret;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void module_setpath(const char *path)
{
  module_path = path;
}

/* ------------------------------------------------------------------------ *
 * Reload a module.                                                         *
 * ------------------------------------------------------------------------ */
int module_reload(struct module *mptr)
{
  char fn[64];

  /* Unload old module */
  mptr->unload(mptr);
#ifdef USE_DSO
  dlclose(mptr->handle);

  /* now open the DSO */
  mptr->handle = dlopen(mptr->path, RTLD_NOW);

  if(mptr->handle == NULL)
  {
    log(module_log, L_warning, "DL error: %s", dlerror());
    module_delete(mptr);
    return -1;
  }

  str_snprintf(fn, sizeof(fn), "%s_load", mptr->name);
  mptr->load = dlsym(mptr->handle, fn);
  str_snprintf(fn, sizeof(fn), "%s_unload", mptr->name);
  mptr->unload = dlsym(mptr->handle, fn);

  if(mptr->load == NULL || mptr->unload == NULL)
  {
    log(module_log, L_warning, "DL error: %s", dlerror());
    dlclose(mptr->handle);
    module_delete(mptr);
    return -1;
  }
#endif
  if(mptr->load(mptr))
  {
    log(module_log, L_warning, "module load() failed.");

#ifdef USE_DSO
    if(mptr->handle)
      dlclose(mptr->handle);
#endif

    module_delete(mptr);
    return -1;
  }

  return 0;
}
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
const char *module_expand(const char *name)
{
  static char ret[256];
  static char lala[32];
  uint32_t    i;
  char       *p = NULL;

  strlcpy(ret, name, sizeof(ret));

  if(strchr(ret, '/') == NULL && (p = strchr(ret, '_'))) {

    for(i = 0; module_table[i][0]; i++) {

      if(!str_ncmp(ret, module_table[i][0], (size_t)p - (size_t)ret)) {
        *p++ = '\0';

        strlcpy(lala, p, sizeof(lala));

        if((p = strchr(lala, '.')))
          *p = '\0';

        strlcpy(ret, module_path, sizeof(lala));
        strlcat(ret, "/", sizeof(ret));
/*        strlcat(ret, module_table[i][1], sizeof(ret));
        strlcat(ret, "/", sizeof(ret));*/
        strlcat(ret, module_table[i][0], sizeof(ret));
        strlcat(ret, "_", sizeof(ret));
        strlcat(ret, lala, sizeof(ret));
        if(DLLEXT[0] != '.')
          strlcat(ret, ".", sizeof(ret));
        strlcat(ret, DLLEXT, sizeof(ret));

        break;
      }
    }
  }

  return ret;
}

/* ------------------------------------------------------------------------ *
 * Initialize module heap.                                                  *
 * ------------------------------------------------------------------------ */
void module_init(void)
{
  module_log = log_source_register("module");

  dlink_list_zero(&module_list);

  module_id = 0;

  mem_static_create(&module_heap, sizeof(struct module), MODULE_BLOCK_SIZE);
  mem_static_note(&module_heap, "module block heap");
}

/* ------------------------------------------------------------------------ *
 * Destroy module heap.                                                     *
 * ------------------------------------------------------------------------ */
void module_shutdown(void)
{
  struct module *module;
  struct module *next;

  dlink_foreach_safe(&module_list, module, next)
    module_delete(module);

  mem_static_destroy(&module_heap);

  log_source_unregister(module_log);
}

/* ------------------------------------------------------------------------ *
 * Add a module.                                                            *
 * ------------------------------------------------------------------------ */
struct module *module_add(const char *path)
{
  struct module *module;

  /* Already loaded? */
  if(module_find_name(path))
    return NULL;

  /* Already loaded? */
  if(module_find_path(module_expand(path)))
    return NULL;

  module = module_new(module_expand(path));

  if(module == NULL)
    return NULL;

  dlink_add_head(&module_list, &module->node, module);

  log(module_log, L_status, "Loaded module: %s", module->path);

  return module;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int module_update(struct module *mptr)
{
  return 0;
}

/* ------------------------------------------------------------------------ *
 * Remove a module.                                                         *
 * ------------------------------------------------------------------------ */
void module_delete(struct module *module)
{
  log(module_log, L_status, "Unloading module: %s", module->path);

  if(module->unload)
    module->unload(module);

#ifdef USE_DSO
  if(module->handle)
    dlclose(module->handle);
#endif

  dlink_delete(&module_list, &module->node);

  mem_static_free(&module_heap, module);
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct module *module_find_path(const char *path)
{
  struct module *module;
  hash_t         hash;
    
  hash = str_hash(path);

  dlink_foreach(&module_list, module)
  {
    if(module->phash == hash)
    {
      if(!str_cmp(module->path, path))
        return module;
    }
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct module *module_find_name(const char *name)
{
  struct module *module;
  hash_t         hash;
    
  hash = str_hash(name);

  dlink_foreach(&module_list, module)
  {
    if(module->nhash == hash)
    {
      if(!str_cmp(module->name, name))
        return module;
    }
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct module *module_find_id(uint32_t id)
{
  struct module *module;

  dlink_foreach(&module_list, module)
  {
    if(module->id == id)
      return module;
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct module *module_pop(struct module *module)
{
  if(module)
  {
    if(!module->refcount)
      log(module_log, L_warning, "Poping deprecated module: %s",
          module->name);

    module->refcount++;
  }

  return module;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct module *module_push(struct module **moduleptr)
{
  if(*moduleptr)
  {
    if((*moduleptr)->refcount == 0)
    {
      log(module_log, L_warning, "Trying to push deprecated user: %s",
          (*moduleptr)->name);
    }
    else
    {
      if(--(*moduleptr)->refcount == 0)
        module_delete(*moduleptr);

      (*moduleptr) = NULL;
    }
  }

  return *moduleptr;
}

/* ------------------------------------------------------------------------ *
 * Dump module list and heap.                                               *
 * ------------------------------------------------------------------------ */
void module_dump(struct module *mptr)
{
  if(mptr == NULL)
  {
    dump(module_log, "[================ module summary ================]");

    dlink_foreach_up(&module_list, mptr)
    {
      dump(module_log, " #%u: [%u] %-20s (%p)",
           mptr->id, mptr->refcount, mptr->name, mptr->handle);
    }

    dump(module_log, "[============= end of module summary ============]");
  }
  else
  {
    dump(module_log, "[================= module dump ==================]");

    dump(module_log, "         id: #%u", mptr->id);
    dump(module_log, "   refcount: %u", mptr->refcount);
    dump(module_log, "      nhash: %p", mptr->nhash);
    dump(module_log, "      phash: %p", mptr->phash);
    dump(module_log, "     handle: %p", mptr->handle);
    dump(module_log, "       load: %p", mptr->load);
    dump(module_log, "     unload: %p", mptr->unload);
    dump(module_log, "       name: %s", mptr->name);
    dump(module_log, "       path: %s", mptr->path);

    dump(module_log, "[============== end of module dump ==============]");
  }
}

#if 0
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int module_dlopen(struct module *mptr)
{
  Elf32_Ehdr *ehdr;
  Elf32_Phdr *phdr;
  char        header[1024];
  void       *libaddr;
  void       *dynamic_addr;
  size_t      dynamic_size;
  int         piclib;
  uint32_t    i;
  size_t      minvma = 0xffffffff;
  size_t      maxvma = 0x00000000;
  int         flags;

  /* Open shared object */
  if((mptr->fd = syscall_open(mptr->path, O_RDONLY)) == -1)
  {
    log(module_log, L_warning, "unable to open shared object: %s", mptr->name);

    return -1;
  }

  /* Read ELF header */
  syscall_read(mptr->fd, header, sizeof(header));

  ehdr = (Elf32_Ehdr *)header;

  /* Check for ELF signature */
  if(ehdr->e_ident[0] != 0x7f ||
     ehdr->e_ident[1] != 'E' ||
     ehdr->e_ident[2] != 'L' ||
     ehdr->e_ident[3] != 'F')
  {
    log(module_log, L_warning, "is not a valid ELF file: %s", mptr->name);
    return -1;
  }

  /* Check for i386 architecture */
  if(ehdr->e_type != ET_DYN ||
     ehdr->e_machine != EM_386)
  {
    log(module_log, L_warning, "is not a valid ELF file: %s", mptr->name);
    return -1;
  }

  phdr = (Elf32_Phdr *)&header[ehdr->e_phoff];
  piclib = 1;

  for(i = 0; i < ehdr->e_phnum; i++, phdr++)
  {
    /* Get dynamic section location and size */
    if(phdr->p_type == PT_DYNAMIC)
    {
      dynamic_addr = (void *)phdr->p_vaddr;
      dynamic_size = phdr->p_filesz;
    }

    /* Get bottom and top of virtual memory */
    if(phdr->p_type == PT_LOAD)
    {
      if(i == 0 && phdr->p_vaddr > 0x1000000)
      {
        piclib = 0;
        minvma = phdr->p_vaddr;
      }

      if(piclib && phdr->p_vaddr < minvma)
        minvma = phdr->p_vaddr;

      if(((size_t)phdr->p_vaddr + phdr->p_memsz) > maxvma)
        maxvma = phdr->p_vaddr + phdr->p_memsz;
    }
  }

  /* Pad addresses */
  maxvma = (maxvma + 0x0fff) & 0xfffff000;
  minvma = minvma & 0xffff0000;

  /* Set mapping flags */
  flags = MAP_PRIVATE;

  if(!piclib)
    flags |= MAP_FIXED;

  /* Map header to memory */
  libaddr = syscall_mmap((piclib ? NULL : (void *)minvma),
                         maxvma - minvma, PROT_NONE, flags, mptr->fd, 0);

  /* Now map all PT_LOAD segments to memory */
  phdr = (Elf32_Phdr *)&header[ehdr->e_phoff];

  for(i = 0; i < ehdr->e_phnum; i++, phdr++)
  {
    if(phdr->p_type == PT_LOAD)
    {
      syscall_mmap(libaddr + (phdr->p_vaddr & 0xfffff000),
                   (phdr->p_vaddr & 0x0fff) + phdr->p_filesz,
                   PROT_READ|PROT_WRITE|PROT_EXEC,
                   flags, mptr->fd,
                   phdr->p_offset & 0x7ffff000);
    }
  }

  return 0;
}
#endif
