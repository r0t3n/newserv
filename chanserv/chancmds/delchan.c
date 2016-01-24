/* Automatically generated by refactor.pl.
 *
 *
 * CMDNAME: delchan
 * CMDLEVEL: QCMD_OPER
 * CMDARGS: 2
 * CMDDESC: Removes a channel from the bot.
 * CMDFUNC: csc_dodelchan
 * CMDPROTO: int csc_dodelchan(void *source, int cargc, char **cargv);
 * CMDHELP: Usage: delchan <channel> [<reason>]
 * CMDHELP: Removes a channel from the bot, reason is optional.
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

int csc_dodelchan(void *source, int cargc, char **cargv) {
  nick *sender=source;
  reguser *rup=getreguserfromnick(sender);
  chanindex *cip;
  regchan *rcp;

  if (!rup)
    return CMD_ERROR;

  if (cargc<1) {
    chanservstdmessage(sender, QM_NOTENOUGHPARAMS, "delchan");
    return CMD_ERROR;
  }

  if (!(cip=findchanindex(cargv[0])) || !(rcp=cip->exts[chanservext])) {
    chanservstdmessage(sender, QM_UNKNOWNCHAN, cargv[0]);
    return CMD_ERROR;
  }

  if (rcp->ID == lastchannelID) {
    chanservsendmessage(sender, "Sorry, can't delete last channel -- wait a while and try again.");
    return CMD_ERROR;
  }

  cs_log(sender,"DELCHAN %s (%s)",cip->name->content,cargc>1?cargv[1]:"");
  cs_removechannel(rcp, "Channel deleted.");
  chanservstdmessage(sender, QM_DONE);

  return CMD_OK;
}
