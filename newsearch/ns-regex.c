/*
 * REGEX functionality
 */

#include "newsearch.h"

#include <stdio.h>
#include <stdlib.h>

#include <pcre.h>

struct regex_localdata {
  struct searchNode *targnode;
  struct searchNode *patnode;
  pcre *pcre;
  pcre_extra *pcre_extra;
};

void *regex_exe(struct searchNode *thenode, int type, void *theinput);
void regex_free(struct searchNode *thenode);

struct searchNode *regex_parse(int type, int argc, char **argv) {
  struct regex_localdata *localdata;
  struct searchNode *thenode;
  struct searchNode *targnode, *patnode;
  const char *err;
  int erroffset;
  pcre *pcre;
  pcre_extra *pcre_extra;

  if (argc<2) {
    parseError="regex: usage: regex source pattern";
    return NULL;
  }

  if (!(targnode = search_parse(type, argv[0])))
    return NULL;

  if (!(patnode = search_parse(type, argv[1]))) {
    (targnode->free)(targnode);
    return NULL;
  }

  if (!(patnode->returntype & RETURNTYPE_CONST)) {
    parseError="regex: only constant regexes allowed";
    (targnode->free)(targnode);
    (patnode->free)(patnode);
    return NULL;
  }

  if (!(pcre=pcre_compile((char *)(patnode->exe)(patnode,RETURNTYPE_STRING,NULL),
			  PCRE_CASELESS, &err, &erroffset, NULL))) {
    parseError=err;
    return NULL;
  }

  pcre_extra=pcre_study(pcre, 0, &err);

  localdata=(struct regex_localdata *)malloc(sizeof (struct regex_localdata));
    
  localdata->targnode=targnode;
  localdata->patnode=patnode;
  localdata->pcre=pcre;
  localdata->pcre_extra=pcre_extra;

  thenode = (struct searchNode *)malloc(sizeof (struct searchNode));

  thenode->returntype = RETURNTYPE_BOOL;
  thenode->localdata = localdata;
  thenode->exe = regex_exe;
  thenode->free = regex_free;

  return thenode;
}

void *regex_exe(struct searchNode *thenode, int type, void *theinput) {
  struct regex_localdata *localdata;
  char *target;

  localdata = thenode->localdata;
  
  target  = (char *)((localdata->targnode->exe)(localdata->targnode,RETURNTYPE_STRING, theinput));

  if (pcre_exec(localdata->pcre, localdata->pcre_extra,target,strlen(target),0,
		0,NULL,0)) {
    /* didn't match */
    return falseval(type);
  } else {
    return trueval(type);
  }
}

void regex_free(struct searchNode *thenode) {
  struct regex_localdata *localdata;

  localdata=thenode->localdata;

  if (localdata->pcre_extra)
    pcre_free(localdata->pcre_extra);

  if (localdata->pcre)
    pcre_free(localdata->pcre);

  (localdata->patnode->free)(localdata->patnode);
  (localdata->targnode->free)(localdata->targnode);
  free(localdata);
  free(thenode);
}
