/* hooks.h */

#ifndef __HOOKS_H
#define __HOOKS_H

#define HOOKMAX 5000

/* This is the authoritative registry of all known hook numbers */

#define HOOK_CORE_REHASH             0
#define HOOK_CORE_STATSREQUEST       1
#define HOOK_CORE_STATSREPLY         2

#define HOOK_IRC_CONNECTED         100
#define HOOK_IRC_DISCON            101
#define HOOK_IRC_SENDBURSTSERVERS  102
#define HOOK_IRC_SENDBURSTNICKS    103
#define HOOK_IRC_SENDBURSTBURSTS   104

#define HOOK_SERVER_NEWSERVER      200  /* Argument is number of new server */
#define HOOK_SERVER_LOSTSERVER     201  /* Argument is number of lost server */

#define HOOK_NICK_NEWNICK          300  /* Argument is nick* */
#define HOOK_NICK_RENAME           301  /* Argument is nick* */
#define HOOK_NICK_LOSTNICK         302  /* Argument is nick* */
#define HOOK_NICK_WHOISCHANNELS    303  /* Argument is nick*[2] (sender, target) */
#define HOOK_NICK_ACCOUNT          304  /* Argument is nick* */
#define HOOK_NICK_QUIT             305  /* Argument is void*[2] (nick, reason) */
#define HOOK_NICK_SETHOST          306  /* Argument is nick* */

#define HOOK_CHANNEL_BURST         400  /* Argument is channel pointer */
#define HOOK_CHANNEL_CREATE        401  /* Argument is void*[2] (channel, nick) */
#define HOOK_CHANNEL_JOIN          402  /* Argument is void*[2] (channel, nick) */
#define HOOK_CHANNEL_PART          403  /* Argument is void*[3] (channel, nick, reason) */
#define HOOK_CHANNEL_KICK          404  /* Argument is void*[4] (channel, kicked, kicker, reason) ** KICKER COULD BE NULL ***/
#define HOOK_CHANNEL_TOPIC         405  /* Argument is void*[2] (channel, nick) ** NICK COULD BE NULL ** */
#define HOOK_CHANNEL_MODECHANGE    406  /* Argument is void*[3] (channel, nick, flags) ** NICK COULD BE NULL ** */
#define HOOK_CHANNEL_BANSET        407  /* Argument is void*[2] (channel, nick) ** NICK COULD BE NULL **, ban will be first ban on channel */
#define HOOK_CHANNEL_BANCLEAR      408  /* Argument is void*[2] (channel, nick) ** NICK COULD BE NULL **, ban will be gone.  XXX - could we care what the ban was? */
#define HOOK_CHANNEL_OPPED         409  /* Argument is void*[3] (channel, nick, target) ** NICK COULD BE NULL ** */
#define HOOK_CHANNEL_DEOPPED       410  /* Argument is void*[3] (channel, nick, target) ** NICK COULD BE NULL ** */
#define HOOK_CHANNEL_VOICED        411  /* Argument is void*[3] (channel, nick, target) ** NICK COULD BE NULL ** */
#define HOOK_CHANNEL_DEVOICED      412  /* Argument is void*[3] (channel, nick, target) ** NICK COULD BE NULL ** */

#define HOOK_CHANNEL_NEWCHANNEL    413  /* Argument is channel pointer */
#define HOOK_CHANNEL_LOSTCHANNEL   414  /* Argument is channel pointer */

#define HOOK_CHANNEL_NEWNICK       415  /* Argument is void*[2] (channel, nick) */
#define HOOK_CHANNEL_LOSTNICK      416  /* Argument is void*[2] (channel, nick) */

#define HOOK_CHANSERV_DBLOADED     500  /* No arg */

typedef void (*HookCallback)(int, void *);

void inithooks();
int registerhook(int hooknum, HookCallback callback);
int deregisterhook(int hooknum, HookCallback callback);
void triggerhook(int hooknum, void *arg);

#endif