/* chaosircd - Chaoz's IRC daemon daemon
 *
 * Copyright (C) 2003  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: um_paranoid.c,v 1.4 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/hook.h"
#include "libchaos/ssl.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/user.h"
#include "ircd/server.h"
#include "ircd/client.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/usermode.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void um_paranoid_whois_hook (struct client         *cptr,
                                    struct user           *uptr);

int         um_paranoid_bounce     (struct user           *uptr,
                                    struct usermodechange *umcptr,
                                    uint32_t               flags);


/* -------------------------------------------------------------------------- *
 * Locals                                                                     *
 * -------------------------------------------------------------------------- */
static struct usermode um_paranoid =
{
  'p',
  USERMODE_LIST_OFF,
  USERMODE_LIST_LOCAL,
  USERMODE_ARG_DISABLE,
  um_paranoid_bounce
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int um_paranoid_load(void)
{
  if(usermode_register(&um_paranoid))
    return -1;

  hook_register(user_whois, HOOK_DEFAULT, um_paranoid_whois_hook);

  server_default_caps |= CAP_PAR;

  return 0;
}

void um_paranoid_unload(void)
{
  server_default_caps &= ~CAP_PAR;

  hook_unregister(user_whois, HOOK_DEFAULT, um_paranoid_whois_hook);
  usermode_unregister(&um_paranoid);

}

int um_paranoid_bounce(struct user *uptr, struct usermodechange *umcptr,
                       uint32_t flags)
{
  if(flags & USERMODE_OPTION_PERMISSION)
    return -1;

  return 0;
}

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
static void um_paranoid_whois_hook(struct client *cptr,
                                   struct user   *uptr)
{
  char cipher[64];

  ssl_cipher(cptr->source->fds[0], cipher, sizeof(cipher));

  if(uptr->modes & ((int)'p' - 0x40))
    numeric_send(cptr, RPL_WHOISSSL, uptr->client->name, cipher);
}
