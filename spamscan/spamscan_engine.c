#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "../core/error.h"
#include "../lib/irc_string.h"
#include "spamscan.h"
#include "spamscan_array.h"
#include "spamscan_engine.h"
#include "dmp.h"

int spamscan_process_diff(void *ref, dmp_operation_t op, const void *data, uint32_t len);

ss_profile ssdefaultprofile = {1, 60, 15, 20, 30, 35, 50};

ss_points sspoints = {12, 12, 12, 20, 12, 15, 20, 20, 20}; 

float spamscan_loop_line(char *line) {

  if (!strlen(line))
    return 0;

  int b = 0;
  int bc = 0;
  int u = 0;
  int uc = 0;
  int r = 0;
  int rc = 0;
  int caps = 0;
  int fg = 0;
  int fgc = 0;
  int bg = 0;
  int bgc = 0;
  int total = 0;

  int i;
  int length = strlen(line);
  char ch;

  for (i = 0; i < length; i++) {
    ch = ((char*)line)[i];
    if (ch == '\002')
      if (b)
        b = 0;
      else
        b = 1;
    else if (ch == '\037')
      if (u)
        u = 0;
      else
        u = 1;
    else if (ch == '\026')
      if (r)
        r = 0;
      else
        r = 1;
    else if (ch == '\027') {
      b = 0;
      u = 0;
      r = 0;
      fg = 0;
      bg = 0;
    } else if (ch == '\003') {
      ch = ((char*)line)[i+1];
      if (!isdigit(ch)) {
        fg = 0;
        bg = 0;
        continue;
      }
      fg = 1;
      i++;
      ch = ((char*)line)[i+1];
      if (isdigit(ch)) {
        i++;
        ch = ((char*)line)[i+1];
      }
      if (ch != ',')
        continue;
      i++;
      ch = ((char*)line)[i+1];
      if (!isdigit(ch)) {
        bg = 0;
        continue;
      }
      i++;
      bg = 1;
      ch = ((char*)line)[i+1];
      if (isdigit(ch))
        i++;
    } else {
      if (isspace(ch))
        continue;
      if (isupper(ch))
        caps++;
      if (b)
        bc++;
      if (u)
        uc++;
      if (r)
        rc++;
      if (fg)
        fgc++;
      if (bg)
        bgc++;
      total++;
    }
  }

  if (!bc && !uc && !rc && !caps && !fgc && !bgc)
    return 0;

  ss_profile *profile;
  ss_points *points;

  profile = &ssdefaultprofile;
  points = &sspoints;

  float p = 0;

  p += spamscan_calc_controlcodes(points->bold, profile->threshold, bc, total);
  p += spamscan_calc_controlcodes(points->bold, profile->threshold, uc, total);
  p += spamscan_calc_controlcodes(points->bold, profile->threshold, rc, total);
  p += spamscan_calc_controlcodes(points->bold, profile->threshold, caps, total);
  p += spamscan_calc_controlcodes(points->bold, profile->threshold, fgc, total);
  p += spamscan_calc_controlcodes(points->bold, profile->threshold, bgc, total);

  return p;
}

int spamscan_check_repeat(const char *new, const char *old) {
  dmp_diff *diff;
  int d = 0;

  if (dmp_diff_from_strs(&diff, NULL, new, old) != 0)
    return 0;
  else
    dmp_diff_foreach(diff, spamscan_process_diff, &d);
    dmp_diff_free(diff);
    return d;
}

int spamscan_process_diff(void *ref, dmp_operation_t op, const void *data, uint32_t len) {
  int *sum = ref;
  if (op == DMP_DIFF_EQUAL)
    (*sum) += len;
  return 0;
}

float spamscan_calc_controlcodes(int points, int threshold, int count, int total) {
  if ((!points) || (!threshold) || (!count) || (!total))
    return 0;
  if (total < 10)
    total = 10;
  /*return (((points/100.00)*threshold)*((total-count)/100.00));*/
  return ((((points/100.00)*threshold)/total)*(total-(total-count)));
          /*+((points/100.00)*threshold);*/
}

void ss_decrease(void) {
  int c, u;
  float points;
  ss_profile *profile;
  ss_channel *chn;
  ss_user *usr;

  time_t ts = time(NULL);

  for (c = 0; c < sschannels.cursi; c++) {
    chn = &((ss_channel*)sschannels.content)[c];
    profile = &ssdefaultprofile;

    for (u = 0; u < ssusers.cursi; u++) {
      usr = &((ss_user*)ssusers.content)[u];

      if (match2strings(usr->cp->index->name->content, chn->name->content)) {
        if (usr->points <= 0.10)
          usr->points = 0;
        points = usr->points - ((usr->points/100.00)*profile->decrease);
        if (points <= 0.10)
          usr->points = 0;
        else
          usr->points = points;
      }

      if (difftime(ts, usr->repeat->ts) >= 60) {
        freesstring(usr->repeat->line);
        free(usr->repeat);
        usr->repeat = NULL;
      }

    }
  }
}
