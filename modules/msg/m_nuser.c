/* chaosircd - Chaoz's IRC daemon daemon
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
 * $Id: m_nuser.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/dlink.h"
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/log.h"
#include "libchaos/str.h"
#include "libchaos/timer.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/usermode.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/client.h"
#include "ircd/server.h"
#include "ircd/chars.h"
#include "ircd/user.h"
#include "ircd/msg.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void ms_nuser(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *ms_nuser_help[] =
{
  "NUSER <nick> <hops> <ts> <umodes> <user> <host> <ip> <server> <uid> <info>",
  "",
  "Introduces a client to a remote server.",
  NULL
};

static struct msg ms_nuser_msg = {
  "NUSER", 10, 10, MFLG_SERVER,
  { NULL, NULL, ms_nuser, NULL },
  ms_nuser_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_nuser_load(void)
{
  if(msg_register(&ms_nuser_msg) == NULL)
    return -1;

  return 0;
}

void m_nuser_unload(void)
{
  msg_unregister(&ms_nuser_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0]  - prefix                                                          *
 * argv[1]  - 'client'                                                        *
 * argv[2]  - nickname                                                        *
 * argv[3]  - hopcount                                                        *
 * argv[4]  - tsinfo                                                          *
 * argv[5]  - umodes                                                          *
 * argv[6]  - user                                                            *
 * argv[7]  - host                                                            *
 * argv[8]  - hostip                                                          *
 * argv[9]  - server                                                          *
 * argv[10] - uid                                                             *
 * argv[11] - info                                                            *
 * -------------------------------------------------------------------------- */
static void ms_nuser(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  struct client *acptr;
  struct server *sptr;
  struct user   *uptr;
  time_t         ts;
  char           newnick[IRCD_NICKLEN + 1];

  if(!client_is_server(cptr))
    return;

  /* Check UID collision */
  if((uptr = user_find_uid(argv[10])))
  {
    log(server_log, L_warning, "UID collision");

    client_send(cptr, "KILL %s :UID collision", argv[10]);
    return;
  }

  /* Validate nickname */
  if(!chars_valid_nick(argv[2]))
  {
    log(server_log, L_warning, "User has invalid nickname: %s",
        argv[2]);

    client_send(cptr, "KILL %s :Invalid nickname", argv[10]);
    return;
  }

  /* Validate username */
  if(!chars_valid_user(argv[6]))
  {
    log(server_log, L_warning, "User %s has invalid username: %s",
        argv[2], argv[6]);

    client_send(cptr, "KILL %s :Invalid username", argv[10]);
    return;
  }

  if((sptr = server_find_name(argv[9])) == NULL)
  {
    log(server_log, L_warning, "Client %s from invalid origin %s.",
        argv[2], argv[9]);
    client_send(cptr, "KILL %s :Invalid origin", argv[10]);
    return;
  }

  ts = str_toul(argv[4], NULL, 10);

  /* Check nick collision */
  if((acptr = client_find_nick(argv[2])))
  {
    /* Remote TS is newer, client on remote side looses nick */
    if(acptr->ts <= ts)
    {
      ts = timer_systime;

      if(client_nick_rotate(argv[2], newnick))
        client_nick_random(newnick);

      client_send(cptr, ":%S NBOUNCE %s %s :%lu",
                  server_me, argv[10], newnick, ts);

      argv[2] = newnick;
    }
  }

  /* ...*/

  /* Register and introduce the remote client */
  uptr = user_new(argv[6], argv[10]);

  uptr->client = client_new_remote(CLIENT_USER, lcptr, uptr, sptr->client,
                                   argv[2], atoi(argv[3]),
                                   argv[7], argv[8], argv[11]);

  usermode_make(uptr, &argv[5], NULL,
                USERMODE_OPTION_LINKALL |
                USERMODE_OPTION_SINGLEARG);

  acptr = uptr->client;
  acptr->user = user_pop(uptr);
  acptr->ts = ts;

  cptr->server->in.clients++;

  client_introduce(lcptr, cptr, acptr);
}

