/* Automatically generated by refactor.pl.
 *
 *
 * CMDNAME: rejoin
 * CMDLEVEL: QCMD_OPER
 * CMDARGS: 1
 * CMDDESC: Makes the bot rejoin a channel.
 * CMDFUNC: csc_dorejoin
 * CMDPROTO: int csc_dorejoin(void *source, int cargc, char **cargv);
 * CMDHELP: Usage: rejoin <channel>
 * CMDHELP: Makes the bot rejoin the specified channel.
 */

#include "../chanserv.h"
#include "../../nick/nick.h"
#include "../../lib/flags.h"
#include "../../lib/irc_string.h"
#include "../../channel/channel.h"
#include "../../parser/parser.h"
#include "../../irc/irc.h"
#include "../../localuser/localuserchannel.h"
#include <string.h>
#include <stdio.h>

int csc_dorejoin(void *source, int cargc, char **cargv) {
  nick *sender=source;
  chanindex *cip;
  regchan *rcp;

  if (cargc<1) {
    chanservstdmessage(sender, QM_NOTENOUGHPARAMS, "rejoin");
    return CMD_ERROR;
  }

  if (!(cip=findchanindex(cargv[0])) || !(rcp=cip->exts[chanservext])) {
    chanservstdmessage(sender, QM_UNKNOWNCHAN, cargv[0]);
    return CMD_ERROR;
  }

  if (CIsJoined(rcp) && !CIsSuspended(rcp)) {
    CSetSuspended(rcp);
    chanservjoinchan(cip->channel);
    CClearSuspended(rcp);
    chanservjoinchan(cip->channel);
  }

  chanservstdmessage(sender, QM_DONE);

  return CMD_OK;
}
