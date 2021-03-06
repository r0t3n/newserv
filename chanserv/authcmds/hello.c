/* Automatically generated by refactor.pl.
 *
 *
 * CMDNAME: hello
 * CMDLEVEL: QCMD_NOTAUTHED
 * CMDARGS: 2
 * CMDDESC: Creates a new user account.
 * CMDFUNC: csa_dohello
 * CMDPROTO: int csa_dohello(void *source, int cargc, char **cargv);
 * CMDHELP: Usage: HELLO <email> <email>
 * CMDHELP: Creates a new user account for yourself.  Your current nickname will be used
 * CMDHELP: for the name of the account, and may only contain letters, numbers and 
 * CMDHELP: hyphens (-).  An email containing password details will be sent to the email
 * CMDHELP: address supplied.  Where:
 * CMDHELP: email    - your email address.  Must be entered the same way both times.
 */

#include "../chanserv.h"
#include "../authlib.h"
#include "../../lib/irc_string.h"
#include <stdio.h>
#include <string.h>

int csa_dohello(void *source, int cargc, char **cargv) {
  nick *sender=source;
  reguser *rup;
  char userhost[USERLEN+HOSTLEN+2];
  maildomain *mdp, *smdp;
  char *local;
  reguser *ruh;
  int found=0;
  char *dupemail;
  activeuser *aup;
  maillock *mlp;

  if (getreguserfromnick(sender))
    return CMD_ERROR;

  if (cargc<2) {
    chanservstdmessage(sender, QM_NOTENOUGHPARAMS, "hello");
    return CMD_ERROR;
  }

  if (findreguserbynick(sender->nick)) {
    chanservstdmessage(sender, QM_AUTHNAMEINUSE, sender->nick);
    return CMD_ERROR;
  }

  if (!(aup = getactiveuserfromnick(sender)))
    return CMD_ERROR;

  if (aup->helloattempts > MAXHELLOS) {
    chanservstdmessage(sender, QM_MAXHELLOLIMIT);
    return CMD_ERROR;
  }

  if (strcmp(cargv[0],cargv[1])) {
    chanservstdmessage(sender, QM_EMAILDONTMATCH);
    cs_log(sender,"HELLO FAIL username %s email don't match (%s vs %s)",sender->nick,cargv[0],cargv[1]);
    return CMD_ERROR;
  }

  if (csa_checkeboy(sender, cargv[0]))
    return CMD_ERROR;

  if (csa_checkaccountname(sender, sender->nick))
    return CMD_ERROR;

  for(mlp=maillocks;mlp;mlp=mlp->next) {
    if(!match(mlp->pattern->content, cargv[0])) {
      chanservstdmessage(sender, QM_MAILLOCKED);
      return CMD_ERROR;
    }
  }

  dupemail = strdup(cargv[0]);
  local=strchr(dupemail, '@');
  if(!local) {
    free(dupemail);
    return CMD_ERROR;
  }
  *(local++)='\0';

  mdp=findnearestmaildomain(local);
  if(mdp) {
    for(smdp=mdp; smdp; smdp=smdp->parent) {
      if(MDIsBanned(smdp)) {
        free(dupemail);
        chanservstdmessage(sender, QM_MAILLOCKED);
        return CMD_ERROR;
      }
      if((smdp->count >= smdp->limit) && (smdp->limit > 0)) {
        free(dupemail);
        chanservstdmessage(sender, QM_DOMAINLIMIT);
        return CMD_ERROR;
      }
    }
  }

  mdp=findmaildomainbydomain(local);
  if(mdp) {
    for (ruh=mdp->users; ruh; ruh=ruh->nextbydomain) {
      if (ruh->localpart)
        if (!strcasecmp(dupemail, ruh->localpart->content)) {
          found++;
        }
    }

    if((found >= mdp->actlimit) && (mdp->actlimit > 0)) {
      free(dupemail);
      chanservstdmessage(sender, QM_ADDRESSLIMIT);
      return CMD_ERROR;
    }
  }

  free(dupemail);

  aup->helloattempts++;

  rup=csa_createaccount(sender->nick,"", cargv[0]);
  csa_createrandompw(rup->password, PASSLEN);
  sprintf(userhost,"%s@%s",sender->ident,sender->host->name->content);
  rup->lastuserhost=getsstring(userhost,USERLEN+HOSTLEN+1);

  chanservstdmessage(sender, QM_NEWACCOUNT, rup->username,rup->email->content);
  cs_log(sender,"HELLO OK created auth %s (%s)",rup->username,rup->email->content); 
  csdb_createuser(rup);
  csdb_createmail(rup, QMAIL_NEWACCOUNT);

  return CMD_OK;
}
