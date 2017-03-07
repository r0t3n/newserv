/* spamscan service */

/*
   makes use of:
     *shroud's request service module as a base
     *github.com/udif/diff-match-patch-c for repeat checking
*/

/*
   todo:
    *implement proper database for channels and profiles (aka scrap chanfile?)
    *implement proper profile logic for easier management
    *implement new/missing detection mechanisms (repeat/advertisement/etc)
    *implement user tracking through quits to detect evasion attempts?
    *implement punishment mechanisms, both local (per-channel) and global
    *implement nterfacer for interaction with webinterface for user management
    *tidy up and optimize code
*/

#include <stdio.h>
#include <string.h>
#include "../localuser/localuser.h"
#include "../localuser/localuserchannel.h"
#include "../core/config.h"
#include "../core/schedule.h"
#include "../lib/irc_string.h"
#include "../lib/splitline.h"
#include "../lib/version.h"
#include "../control/control.h"
#include "../splitlist/splitlist.h"
#include "../channel/channel.h"
#include "spamscan.h"
#include "spamscan_array.h"
#include "spamscan_engine.h"

MODULE_VERSION("");

nick *ssnick;
CommandTree *sscommands;

void ss_registeruser(void);
void ss_handler(nick *target, int type, void **args);

int sscmd_showcommands(void *user, int cargc, char **cargv);
int sscmd_stats(void *user, int cargc, char **cargv);
int sscmd_addchannel(void *user, int cargc, char **cargv);
int sscmd_remchannel(void *user, int cargc, char **cargv);
int sscmd_listchannels(void *user, int cargc, char **cargv);
int sscmd_findchannel(void *user, int cargc, char **cargv);
int sscmd_decrease(void *user, int cargc, char **cargv);
#define min(a,b) ((a > b) ? b : a)

/* stats counters */
int ss_processed = 0;
int ss_banned = 0;
int ss_kicked = 0;
int ss_fwarned = 0;
int ss_warned = 0;
int ss_unknown = 0;

/* log fd */
FILE *ss_logfd;

/* config */
sstring *ss_nick, *ss_ident, *ss_host, *ss_real, *ss_auth;
int ss_authid;

static int extloaded = 0;

void _init(void) {
  sstring *m;

  if (!ss_initarrays())
    return;

  extloaded = 1;

  ss_nick = getcopyconfigitem("spamscan", "nick", "S", BUFSIZE);
  ss_ident = getcopyconfigitem("spamscan", "ident", "S", BUFSIZE);
  ss_host = getcopyconfigitem("spamscan", "host", "spamscan.example.com", BUFSIZE);
  ss_real = getcopyconfigitem("spamscan", "real", "Spamscan", BUFSIZE);
  ss_auth = getcopyconfigitem("spamscan", "auth", "S", BUFSIZE);

  m = getconfigitem("spamscan", "authid");
  if (!m)
    ss_authid = 2891722;
  else
    ss_authid = atoi(m->content);

  sscommands = newcommandtree();

  addcommandtotree(sscommands, "showcommands", SSU_ANY, 0, &sscmd_showcommands);
  addcommandtotree(sscommands, "stats", SSU_OPER, 0, &sscmd_stats);
  addcommandtotree(sscommands, "addchannel", SSU_OPER, 2, &sscmd_addchannel);
  addcommandtotree(sscommands, "remchannel", SSU_OPER, 1, &sscmd_remchannel);
  addcommandtotree(sscommands, "listchannels", SSU_OPER, 1, &sscmd_listchannels); 
  addcommandtotree(sscommands, "findchannel", SSU_OPER, 1, &sscmd_findchannel);
  addcommandtotree(sscommands, "decrease", SSU_OPER, 0, &sscmd_decrease);

  ss_logfd = fopen("logs/spamscan.log", "a");

  scheduleoneshot(time(NULL) + 1, (ScheduleCallback)&ss_registeruser, NULL);
  schedulerecurring(time(NULL) + 70, 0, 60, (ScheduleCallback)&ss_decrease, NULL);
}

void _fini(void) {
  if(!extloaded)
    return;

  deregisterlocaluser(ssnick, NULL);

  deletecommandfromtree(sscommands, "showcommands", &sscmd_showcommands);
  deletecommandfromtree(sscommands, "stats", &sscmd_stats);
  deletecommandfromtree(sscommands, "addchannel", &sscmd_addchannel);
  deletecommandfromtree(sscommands, "remchannel", &sscmd_remchannel);
  deletecommandfromtree(sscommands, "listchannels", &sscmd_listchannels);
  deletecommandfromtree(sscommands, "findchannel", &sscmd_findchannel);
  deletecommandfromtree(sscommands, "decrease", &sscmd_decrease);

  destroycommandtree(sscommands);

  deleteallschedules((ScheduleCallback)&ss_decrease);
  ss_finiarrays();

  freesstring(ss_nick);
  freesstring(ss_ident);
  freesstring(ss_host);
  freesstring(ss_real);
  freesstring(ss_auth);

  if (ss_logfd != NULL)
    fclose(ss_logfd);

  deleteallschedules((ScheduleCallback)&ss_registeruser);
}

void ss_registeruser(void) {
  channel *cp;

  ssnick = registerlocaluserflags(ss_nick->content, ss_ident->content, ss_host->content,
                             ss_real->content, ss_auth->content, ss_authid, 0,
                             UMODE_ACCOUNT | UMODE_SERVICE | UMODE_OPER,
                             ss_handler);

  cp = findchannel("#twilightzone");

  if (cp == NULL)
    localcreatechannel(ssnick, "#twilightzone");
  else {
    localjoinchannel(ssnick, cp);
    localgetops(ssnick, cp);
  }

  int i;
  ss_channel chn;

  for (i = sschannels.cursi - 1; i >= 0; i--) {
    chn = ((ss_channel*)sschannels.content)[i];
    cp = findchannel(chn.name->content);

    if (cp == NULL)
      localcreatechannel(ssnick, chn.name->content);
    else
      localjoinchannel(ssnick, cp);
      localgetops(ssnick, cp);

  }

}

void ss_handler(nick *target, int type, void **params) {
  Command* cmd;
  nick* user;
  channel *cp;
  char* line;
  int cargc;
  char* cargv[30];

  ss_channel *chn;
  ss_profile *profile;

  switch (type) {
    case LU_CHANMSG:

      user = getnickbynick((char*)params[0]);
      cp = (channel*)params[1];
      line = (char*)params[2];

      if (!cp)
        break;

      chn = ss_findchannel(cp->index->name->content);

      if (chn == NULL)
        break;

      if (IsService(user))
        break;

      ss_user *usr = ss_finduser(user, cp);

      if (usr == NULL) {
        int slot = array_getfreeslot(&ssusers);
        usr = &(((ss_user*)ssusers.content)[slot]);

        usr->np = user;
        usr->cp = cp;
      }

      float r = spamscan_loop_line(line);
      profile = &ssdefaultprofile;

      usr->points += r;
      int rr = 0;
      struct timeval tv;
      gettimeofday(&tv, NULL);

      if (usr->repeat == NULL) {
        usr->repeat = malloc(sizeof(ss_repeat));
        usr->repeat->line = getsstring(line, strlen(line));
        usr->repeat->ts = &tv.tv_sec;
      } else {
        if (&tv.tv_sec - usr->repeat->ts < 60) {
          if (line != NULL && usr->repeat->line->content != NULL)
            rr = spamscan_check_repeat(line, usr->repeat->line->content);
        } else {
          rr = -1;
        }
        freesstring(usr->repeat->line);
        usr->repeat->line = getsstring(line, strlen(line));
        usr->repeat->ts = &tv.tv_sec;
      }

      if (IsOper(user))
        sendnoticetouser(ssnick, user, "Line Length %zu, Line Points %.2f, Repeat Diff %d, User points %.2f", strlen(line), r, rr, usr->points);

      ss_processed++;
      chn->processed++;

      if (usr->points <= 0) /* not a monitored channel and/or user */
          break;
      if (usr->points >= profile->ban) /* ban user from channel */
          ss_banned++; chn->banned++; break;
      if (usr->points >= profile->kick) /* kick user from channel */
          ss_kicked++; chn->kicked++; break;
      if (usr->points >= profile->fwarn) /* give user final warning */
          ss_fwarned++; chn->fwarned++; break;
      if (usr->points >= profile->warn) /* give user a warning */
          ss_warned++; chn->warned++; break;

      /* we should never hit this*/
      ss_unknown++; break;

    case LU_PRIVMSG:
    case LU_SECUREMSG:
      user = params[0];
      line = params[1];
      cargc = splitline(line, cargv, 30, 0);

      if (cargc == 0)
        return;

      cmd = findcommandintree(sscommands, cargv[0], 1);

      if (cmd == NULL) {
        sendnoticetouser(ssnick, user, "Unknown command.");

        return;
      }

      if ((cmd->level & SSU_OPER) && !IsOper(user)) {
        sendnoticetouser(ssnick, user, "Sorry, this command is not "
              "available to you.");

        return;
      }

      if (cargc - 1 > cmd->maxparams)
        rejoinline(cargv[cmd->maxparams], cargc - cmd->maxparams);

      /* handle the command */
      cmd->handler((void*)user, min(cargc - 1, cmd->maxparams), &(cargv[1]));

      break;
    case LU_KILLED:
      scheduleoneshot(time(NULL) + 5, (ScheduleCallback)&ss_registeruser, NULL);

      break;
    case LU_PRIVNOTICE:
      break;
  }
}

int sscmd_showcommands(void *user, int cargc, char **cargv) {
  nick* np = (nick*)user;
  int n, i;
  Command* cmdlist[50];

  n = getcommandlist(sscommands, cmdlist, 50);

  sendnoticetouser(ssnick, np, "Available commands:");
  sendnoticetouser(ssnick, np, "-------------------");

  for (i = 0; i < n; i++) {
    if ((cmdlist[i]->level & SSU_OPER) && !IsOper(np))
      continue;

    sendnoticetouser(ssnick, np, "%s", cmdlist[i]->command->content);
  }

  sendnoticetouser(ssnick, np, "End of SHOWCOMMANDS");

  return 0;
}

int sscmd_stats(void *user, int cargc, char **cargv) {
  nick *np = (nick*)user;

  if (!IsOper(np)) {
    sendnoticetouser(ssnick, np, "You do not have access to this command.");

    return SS_ERROR;
  }

  sendnoticetouser(ssnick, np, "Processed:		%d", ss_processed);
  sendnoticetouser(ssnick, np, "Warned:			(%d/%d)", ss_warned, ss_fwarned);
  sendnoticetouser(ssnick, np, "Kicked:			%d", ss_kicked);
  sendnoticetouser(ssnick, np, "Banned:			%d", ss_banned);
  sendnoticetouser(ssnick, np, "Unknown:		%d", ss_unknown);

  return SS_OK;
}

int sscmd_addchannel(void *user, int cargc, char **cargv) {
  nick *np = (nick*)user;
  int profile = 0;

  if (!IsOper(np)) {
    sendnoticetouser(ssnick, np, "You do not have access to this command.");

    return SS_ERROR;
  }

  if (cargc < 1) {
    sendnoticetouser(ssnick, np, "Syntax: addchannel <#channel> [profileid]");

    return SS_ERROR;
  }

  if (!ss_addchannel(cargv[0], profile)) {
    sendnoticetouser(ssnick, np, "Channel %s is already known to me.", cargv[0]);

    return SS_ERROR;
  } else {

    channel *cp = findchannel(cargv[0]);
    if (cp == NULL) {
      localcreatechannel(ssnick, cargv[0]);
    } else {
      localjoinchannel(ssnick, cp);
      localgetops(ssnick, cp);
    }

    sendnoticetouser(ssnick, np, "Channel %s successfully added.", cargv[0]);

    return SS_OK;
  }
}

int sscmd_remchannel(void *user, int cargc, char **cargv) {
  nick *np = (nick*)user;

  if (!IsOper(np)) {
    sendnoticetouser(ssnick, np, "You do not have access to this command.");

    return SS_ERROR;
  }

  if (cargc < 1) {
    sendnoticetouser(ssnick, np, "Syntax: remchannel <#channel>");

    return SS_ERROR;
  }

  if (!ss_remchannel(cargv[0])) {
    sendnoticetouser(ssnick, np, "Channel %s is not known to me.", cargv[0]);

    return SS_ERROR;
  } else {

    channel *cp = findchannel(cargv[0]);
    if (cp != NULL) {
      localpartchannel(ssnick, cp, "Channel removed.");
    }

    sendnoticetouser(ssnick, np, "Channel %s successfully removed.", cargv[0]);

    return SS_OK;
  }
}

int sscmd_findchannel(void *user, int cargc, char **cargv) {
  nick *np = (nick*)user;

  if (!IsOper(np)) {
    sendnoticetouser(ssnick, np, "You do not have access to this command.");

    return SS_ERROR;
  }

  if (ss_findchannel(cargv[0]) != NULL)
    sendnoticetouser(ssnick, np, "Channel %s is known to me.", cargv[0]);
  else
    sendnoticetouser(ssnick, np, "Channel %s is not known to me", cargv[0]);

  return SS_OK;
}

int sscmd_listchannels(void *user, int cargc, char **cargv) {
  nick *np = (nick*)user;

  if (!IsOper(np)) {
    sendnoticetouser(ssnick, np, "You do not have access to this command.");

    return SS_ERROR;
  }

  int i;
  ss_channel chn;

  sendnoticetouser(ssnick, np, "Known channels:");

  for (i = sschannels.cursi - 1; i >= 0; i--) {
    chn = ((ss_channel*)sschannels.content)[i];

    sendnoticetouser(ssnick, np, "%s", chn.name->content);
  }

  sendnoticetouser(ssnick, np, "End of LISTCHANNELS (%d known channel(s))", i);

  return SS_OK;
}

int sscmd_decrease(void *user, int cargc, char **cargv) {
  nick *np = (nick*)user;

  if (!IsOper(np)) {
    sendnoticetouser(ssnick, np, "You do not have access to this command.");

    return SS_ERROR;
  }

  sendnoticetouser(ssnick, np, "Starting decrease cycle...");

  ss_decrease();

  sendnoticetouser(ssnick, np, "Finished decrease cycle.");
  return SS_OK;
}

