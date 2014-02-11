/* chaosircd - pi-networks irc server
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
 * $Id: m_geolocation.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/log.h>
#include <libchaos/str.h>
#include <libchaos/dlink.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/msg.h>
#include <chaosircd/user.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/server.h>
#include <chaosircd/lclient.h>
#include <chaosircd/numeric.h>
#include <chaosircd/channel.h>
#include <chaosircd/chanuser.h>
#include <chaosircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_geolocation  (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);
static void ms_geolocation (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_geolocation_help[] = {
  "GEOLOCATION [SET|SEARCH] [geohash]",
  "",
  "Sets the geohash or searches for geohashes",
  NULL
};

static struct msg m_geolocation_msg = {
  "GEOLOCATION", 1, 3, MFLG_CLIENT,
  { NULL, m_geolocation, ms_geolocation, m_geolocation },
  m_geolocation_help
};

static const char base32[] = { 
  '0', '1', '2', '3', '4', '5', '6', '7', '8',
  '9', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'j',
  'k', 'm', 'n', 'p', 'q', 'r', 's', 't', 'u',
  'v', 'w', 'x', 'y', 'z',
  0
};

static int valid_base32(const char *s)
{
  while(*s)
  {
    if(!str_chr(base32, *s))
      return 0;

    s++;
  }
  return 1;
}

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_geolocation_load(void)
{
  if(msg_register(&m_geolocation_msg) == NULL)
    return -1;
  
  return 0;
}

void m_geolocation_unload(void)
{
  msg_unregister(&m_geolocation_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'GEOLOCATION'                                                    *
 * argv[2] - 'SET'                                                            *
 * argv[3] - geohash                                                          *
 * -------------------------------------------------------------------------- */
static void ms_geolocation (struct lclient *lcptr, struct client *cptr,
                            int             argc,  char         **argv)
{
  if(!str_icmp(argv[2], "SET"))
  {
     

    if(client_is_user(cptr))
    {
      strlcpy(cptr->user->name, argv[3], IRCD_USERLEN);

      //log(user_log, L_debug, "Set geolocation for %s to %s", cptr->name, cptr->user->name);    
    }
   
    server_send(lcptr, NULL, CAP_NONE, CAP_NONE, ":%C GEOLOCATION SET :%s", argv[3]);
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'GEOLOCATION'                                                    *
 * -------------------------------------------------------------------------- */
static void m_geolocation(struct lclient *lcptr, struct client *cptr,
                          int             argc,  char         **argv)
{
  int minlen = 9;
  int len;

  if(!valid_base32(argv[3]))
  {
    client_send(cptr, ":%S NOTICE %C :(m_geolocation) invalid geohash '%s': not base32", server_me, cptr, argv[3]);
    return;
  }

  if(!str_icmp(argv[2], "SEARCH"))
    minlen = 2;
    
  len = str_len(argv[3]);

  if(len < minlen)
  {
    client_send(cptr, ":%S NOTICE %C :(m_geolocation) invalid geohash '%s': too short", server_me, cptr, argv[3]);
    return;
  }

  if(!str_icmp(argv[2], "SET"))
  {
    ms_geolocation(lcptr, cptr, argc, argv);
  }
  else if(!str_icmp(argv[2], "SEARCH"))
  {
     struct user *user;
     struct node *node;
     char buffer[IRCD_LINELEN+1];
     size_t di, dlen = 0;
     di = str_snprintf(buffer, sizeof(buffer), ":%S 600 %N", server_me, cptr);

     dlink_foreach_data(&user_list, node, user)
     {
       if(!user->client) continue;
       if(user->client == cptr) continue;
       if(str_len(user->name) < len) continue;
       if(!valid_base32(user->name)) continue;

       if(!str_ncmp(user->name, argv[3], len))
       {
         size_t toklen = str_len(user->client->name) + 1 + str_len(user->name);

         if(di+dlen+1+toklen > IRCD_LINELEN)
         {
           client_send(cptr, "%s", buffer);
           dlen = 0;
         }
         dlen += str_snprintf(&buffer[di+dlen], sizeof(buffer)-(di+dlen), " %N!%U", user->client, user->client);
       }
     }
     if(dlen)
       client_send(cptr, "%s", buffer);

     client_send(cptr, ":%S 601 %s :end of /GEOLOCATION query", server_me, argv[3]);
  }
}
