/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003,2004  Roman Senn <r.senn@nexbyte.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: m_version.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/log.h"
#include "libchaos/module.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/config.h"
#include "ircd/msg.h"
#include "ircd/user.h"
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/server.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/channel.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_version (struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_version_help[] = {
  "VERSION [server]",
  "",
  "Displays version of the given server. If used",
  "without parameters, the information from the",
  "local server is displayed.",
  NULL
};

static struct msg m_version_msg = {
  "VERSION", 0, 1, MFLG_CLIENT,
  { NULL, m_version, m_version, m_version },
  m_version_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_version_load(void)
{
  if(msg_register(&m_version_msg) == NULL)
    return -1;

  return 0;
}

void m_version_unload(void)
{
  msg_unregister(&m_version_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'version'                                                           *
 * -------------------------------------------------------------------------- */
static void m_version(struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv)
{
  if(argc > 2)
  {
    if(server_relay_always(lcptr, cptr, 2, ":%C VERSION :%s", &argc, argv))
      return;
  }

  numeric_send(cptr, RPL_VERSION, PACKAGE_NAME, PACKAGE_VERSION,
#ifdef DEBUG
               "DEBUG",
#else
               "PRODUCTION",
#endif /* DEBUG */
               client_me->name, PACKAGE_RELEASE);

  ircd_support_show(cptr);
}

