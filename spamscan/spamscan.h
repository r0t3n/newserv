#include "../lib/sstring.h"

#define SSU_ANY 0
#define SSU_OPER 1

#define SS_OK 0
#define SS_ERROR 1

#define SS_STATFILE = "data/spamscan.stat"

extern FILE *ss_logfd;

/* config */
extern sstring *ss_nick, *ss_ident, *ss_host, *ss_real, *ss_auth;
extern int ss_authid;
