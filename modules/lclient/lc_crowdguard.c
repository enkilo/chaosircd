
/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/io.h"
#include "libchaos/dlink.h"
#include "libchaos/timer.h"
#include "libchaos/hook.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/lclient.h"
#include "ircd/client.h"
#include "ircd/msg.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static int              lc_crowdguard_hook    (struct lclient   *lcptr);

/* -------------------------------------------------------------------------- *
 * Types                                                                      *
 * -------------------------------------------------------------------------- */
struct lc_sauth {
	struct node node;
	struct lclient *lclient;
	struct timer *timer_auth;
	struct sauth *sauth_userdb;
	int done_auth;
};

/* -------------------------------------------------------------------------- *
 * Local variables                                                            *
 * -------------------------------------------------------------------------- */
static int lc_crowdguard_log;

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int lc_crowdguard_load(void)
{
  lc_crowdguard_log = log_source_find("login");
  if(lc_crowdguard_log == -1)
    lc_crowdguard_log = log_source_register("login");

  if(hook_register(lclient_login, HOOK_DEFAULT, lc_crowdguard_hook) == NULL)
    return -1;

  return 0;
}

void lc_crowdguard_unload(void)
{
  hook_unregister(lclient_login, HOOK_DEFAULT, lc_crowdguard_hook);

  log_source_unregister(lc_crowdguard_log);
  lc_crowdguard_log = -1;
}

/* -------------------------------------------------------------------------- *
 * Hook before user/nick is validated                                         *
 * -------------------------------------------------------------------------- */
static int lc_crowdguard_hook(struct lclient *lcptr)
{
  char info[IRCD_INFOLEN+1];
  size_t i;

  strlcpy(info, lcptr->info, sizeof(info));

  for(i = 0; i < str_len(info); i++)
  {
    unsigned char c = info[i];
    if(c == 0xa0 || c <= 0x20)
      info[i] = ' ';
  }

  log(lc_crowdguard_log, L_status, "User login from %s with handle %s, info: %s",
      lcptr->host, lcptr->name, info);

  return 0;
}

