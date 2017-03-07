#include <stdio.h>
#include <string.h>
#include "../irc/irc.h"
#include "../lib/irc_string.h"
#include "spamscan_array.h"

array sschannels;
array ssusers;

int ss_chld;

int ss_initarrays(void) {
  array_init(&sschannels, sizeof(ss_channel));
  array_setlim1(&sschannels, 5);
  array_setlim2(&sschannels, 20);

  array_init(&ssusers, sizeof(ss_user));
  array_setlim1(&ssusers, 5);
  array_setlim2(&ssusers, 20);

  ss_chld = 0;

  ss_loadchannels();

  return 1;
}

void ss_finiarrays(void) {
  int i;
  ss_channel chn;
  ss_user usr;
  ss_repeat *rpt;

  for (i = 0; i < sschannels.cursi; i++) {
    chn = ((ss_channel*)sschannels.content)[i];

    freesstring(chn.name);

  }

  for (i = 0; i < ssusers.cursi; i++) {
    usr = ((ss_user*)ssusers.content)[i];

    usr.np = NULL;
    usr.cp = NULL;

    rpt = ((ss_repeat*)usr.repeat);
    if (rpt == NULL)
      continue;
    freesstring(rpt->line);
    free(rpt);

  }

  array_free(&sschannels);
  array_free(&ssusers);
}

ss_channel *ss_findchannel(const char *chan) {
  int i;
  ss_channel chn;

  for (i = sschannels.cursi - 1; i >= 0; i--) {
    chn = ((ss_channel*)sschannels.content)[i];

    if (match2strings(chn.name->content, chan)) {
      return &(((ss_channel*)sschannels.content)[i]);
    }
  }

  return NULL;
}

ss_user *ss_finduser(nick *nick, channel *cp) {
  int i;
  ss_user usr;

  if (nick == NULL || cp == NULL)
    return NULL;


  for (i = ssusers.cursi - 1; i >= 0; i--) {
    usr = ((ss_user*)ssusers.content)[i];

    if (usr.np->nick == nick->nick)
      if (usr.cp->index->name == cp->index->name)
        return &(((ss_user*)ssusers.content)[i]);
  }

  return NULL;
}

int ss_addchannel(const char *chan, int profile) {
  int slot;
  ss_channel *chn;

  if (ss_findchannel(chan) != NULL)
    return 0;

  slot = array_getfreeslot(&sschannels);

  chn = &(((ss_channel*)sschannels.content)[slot]);

  chn->name = getsstring(chan, CHANNELLEN);
  chn->profile = profile;
  chn->processed = 0;
  chn->warned = 0;
  chn->fwarned = 0;
  chn->kicked = 0;
  chn->banned = 0;

  ss_savechannels();

  return 1;
}

int ss_remchannel(const char *chan) {
  int i;
  ss_channel chn;

  for (i = 0; i < sschannels.cursi; i++) {
    chn = ((ss_channel*)sschannels.content)[i];

    if (match2strings(chn.name->content, chan)) {
      array_delslot(&sschannels, i);
      ss_savechannels();
      return 1;
    }
  }

  return 0;
}

int ss_parsechanfileline(char *line) {
  char name[CHANNELLEN+1];
  int profile = 1, processed = 0, warned = 0, fwarned = 0, kicked = 0, banned = 0;

  if (sscanf(line, "%s %d %d %d %d %d %d %[^\n]",
             name, &profile, &processed, &warned, &fwarned, &kicked, &banned) < 1)
    return 0;

  if (!ss_addchannel(name, profile))
    return 0;
  else {
    ss_channel *chn = ss_findchannel(name);
    chn->processed = processed;
    chn->warned = warned;
    chn->fwarned = fwarned;
    chn->kicked = kicked;
    chn->banned = 0;

    return 1;
  }
}

int ss_loadchannels() {
  char line [4096];
  FILE *data;
  int count;

  data = fopen(SS_CHANFILE, "r");

  if (data == NULL)
    return 0;

  count = 0;
  ss_chld = 1;

  while (!feof(data)) {
    if (fgets(line, sizeof(line), data) == NULL)
      break;

    if (line[strlen(line) - 1] == '\n')
      line[strlen(line) - 1] = '\0';

    if (line[strlen(line) - 1] == '\r')
      line[strlen(line) - 1] = '\0';

    if (line[0] != '\0') {
      if (ss_parsechanfileline(line))
        count++;
    }
  }

  fclose(data);
  ss_chld = 0;

  return count;
}

int ss_savechannels(void) {
  FILE *data;
  int i, count = 0;

  if (ss_chld)
    return 0;

  data = fopen(SS_CHANFILE, "w");

  if (data == NULL)
    return 0;

  ss_channel chn;

  for (i = 0; i < sschannels.cursi; i++) {
    chn = ((ss_channel*)sschannels.content)[i];

    fprintf(data, "%s %d %d %d %d %d %d\n",
            chn.name->content, chn.processed, chn.profile,
            chn.warned, chn.fwarned, chn.kicked, chn.banned);

    count++;
  }

  fclose(data);

  return count;
}
