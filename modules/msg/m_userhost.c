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
 * $Id: m_userhost.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/log.h"
#include "libchaos/str.h"
#include "libchaos/dlink.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/user.h"
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/numeric.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mr_userhost (struct lclient *lcptr, struct client *cptr,
                         int             argc,  char         **argv);
static void m_userhost  (struct lclient *lcptr, struct client *cptr,
                         int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_userhost_help[] = {
  "USERHOST <nick> [nick] [...]",
  "",
  "Displays the given nicks username, hostname, away status,",
  "operator status and real name in one single line.",
  NULL
};    

static struct msg m_userhost_msg = {
  "USERHOST", 1, 1, MFLG_CLIENT | MFLG_UNREG,
  { mr_userhost, m_userhost, NULL, m_userhost },
  m_userhost_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_userhost_load(void)
{
  if(msg_register(&m_userhost_msg) == NULL)
    return -1;
  
  return 0;
}

void m_userhost_unload(void)
{
  msg_unregister(&m_userhost_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'userhost'                                                       *
 * argv[2] - nick1                                                            *
 * argv[3] - [nick2]                                                          *
 * argv[4] - ...                                                              *
 * -------------------------------------------------------------------------- */
static void mr_userhost(struct lclient *lcptr, struct client *cptr,
                        int             argc,  char         **argv)
{
  return;
}

static void m_userhost(struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv)
{
  struct client *acptr;
  char          *av[64];
  char           result[IRCD_LINELEN - 1];
  size_t         len;
  size_t         n;
  size_t         i;
  int            first = 1;
  
	len = str_snprintf(result, sizeof(result), ":%s 302 %s :",
                 client_me->name, cptr->name);
  
  n = str_tokenize(argv[2], av, 63);
  
  for(i = 0; i < n; i++)
  {
    acptr = client_find_nick(av[i]);
    
    if(acptr == NULL)
      continue;
    
    if(len + 1 + str_len(acptr->name) > IRCD_LINELEN - 2)
      break;
    
    if(!first)
      result[len++] = ' ';
    
    len += strlcpy(&result[len], acptr->name, IRCD_LINELEN - 1 - len);
    
/*    if(acptr->user->modes & UFLG(o))
      result[len++] = '*';*/
    result[len++] = '=';

    if(acptr->user->away[0])
      result[len++] = '-';
    else
      result[len++] = '+';
    
    len += strlcpy(&result[len], acptr->user->name, IRCD_LINELEN - 1 - len);
    result[len++] = '@';
    len += strlcpy(&result[len], acptr->hostreal, IRCD_LINELEN - 1 - len);
    
    first = 0;
  }
  
  client_send(cptr, "%s", result);
}
