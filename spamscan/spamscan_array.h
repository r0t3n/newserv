#include "../nick/nick.h"
#include "../channel/channel.h"
#include <sys/time.h>

typedef struct {
  sstring *name;
  int profile;
  int processed;
  int warned;
  int fwarned;
  int kicked;
  int banned;
} ss_channel;

typedef struct {
  sstring *line;
  time_t ts;
} ss_repeat;

typedef struct {
  nick *np;
  channel *cp;
  ss_repeat *repeat;
  float points;
} ss_user;

extern array sschannels;
extern array ssusers;

#define SS_CHANFILE "data/spamscan.chan"

int ss_initarrays(void);
void ss_finiarrays(void);

int ss_loadchannels(void);
int ss_savechannels(void);

ss_channel *ss_findchannel(const char *name);
ss_user *ss_finduser(nick *nick, channel *cp);

int ss_addchannel(const char *chan, int profile);
int ss_remchannel(const char *chan);
