/* Muhstik, Copyright (C) 2001-2002, Louis Bavoil <mulder@gmx.fr>       */
/*                        2009-2011, Leon Kaiser <literalka@gnaa.eu>    */
/*                                                                      */
/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU Library General Public License  */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU Library General Public License for more details.                 */

/* {{{ Header includes */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <time.h>

#include "../include/control.h"
#include "../include/load.h"
#include "../include/muhstik.h"
#include "../include/print.h"
#include "../include/string.h"
#include "../include/lists.h"
#include "../include/mass.h"
/* }}} */
/* {{{ Variables */
/* {{{ External variables */
extern config_t conf;
extern int mass_ch;
extern int mass_mode;
extern char *mass_server;
extern char *mass_reas;
extern clone_t *cl[];
extern char *channel[];
extern char *chankey[];
extern queue names_op[];
extern queue names[];
extern pthread_mutex_t mutex[];
/* }}} */
/*{{{ Global variables */
int mute;
char *target;
int echo_mode;
/* }}} */
/* }}} */
/* {{{ get*() */
int getchid(char *s)
{
     register int i;

     for (i = 0; i < MAX_CHANS; ++i)
     {
          if (channel[i] && !StrCompare(channel[i], s))
          {
               return i;
          }
     }

     return -1;
}

clone_t *getop(int chid)
{
     register int i;
     clone_t **pcl;
     time_t now;
     now = time(NULL);

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if (is_op(*pcl, chid) && (*pcl)->lastsend != now)
          {
               return *pcl;
          }
     }

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if (is_op(*pcl, chid))
          {
               return *pcl;
          }
     }

     return NULL;
}

clone_t *getone(char *s)
{
     register int i;
     clone_t **pcl;

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if ((*pcl) && (*pcl)->online && !StrCompare((*pcl)->server, s))
          {
               return *pcl;
          }
     }

     return NULL;
}

clone_t *getscan()
{
     register int i;
     clone_t **pcl;

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if ((*pcl) && (*pcl)->online && !(*pcl)->scan)
          {
               return *pcl;
          }
     }

     return NULL;
}
/* }}} */
/* {{{ join_delayed() */
void join_delayed(char *buffer)
{
     int delay;
     register int id;
     clone_t **pcl;
     time_t now;

     if (sscanf(buffer, "%*s %s %d", buffer, &delay) != 2)
     {
          return;
     }

     if ((mass_ch = getchid(buffer)) == -1)
     {
          return;
     }

     now = time(NULL);
     for (id = 0, pcl = cl; id < MAX_CLONES; ++id, ++pcl)
     {
          if ((*pcl) && (*pcl)->online)
          {
               (*pcl)->rejoin_time = now + delay * id;
          }
     }
}
/* }}} */
/* {{{ Parsing and interpreting */
/* {{{ Parsing functions */
void parse_mute(char *buffer, int out)
{
     mute = (mute + 1) & 01;
}

void parse_join(char *buffer, int out)
{
     char tosend[MEDBUF+1];
     char parm[MEDBUF+1];
     int i;
     int delay;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          print(1, 0, out, "Usage: join <chan> [<delay>] [<key>]");
          return;
     }
     if (occur_table(parm, channel, MAX_CHANS))
     {
          print(1, 0, out, "rejoining %s", parm);
          snprintf(tosend, sizeof(tosend), "PART %s\n", parm); /* TODO: Optional PART message/reason. */
          send2clones(tosend);
          pthread_mutex_unlock(&mutex[0]);
          sleep(1);
          pthread_mutex_lock(&mutex[0]);
          snprintf(tosend, sizeof(tosend), "JOIN %s\n", parm);
          send2clones(tosend);
          return;
     }
     if ((i = add_table(parm, channel, MAX_CHANS)) == -1)
     {
          print(1, 0, out, "No more channel available");
          return;
     }
     switch (sscanf(buffer, "%*s %*s %d %"MEDBUF_TXT"s", &delay, parm))
     {
          case 2:
               free(chankey[i]);
               chankey[i] = StrDuplicate(parm);
               /* FALL THROUGH */
          case 1:
               print(1, 0, out, "joining %s with a delay of %d", parm, delay);
               join_delayed(buffer);
               break;
          default:
               print(1, 0, out, "joining %s", parm);
               send2clones(buffer);
     }
}

void parse_part(char *buffer, int out)
{
     char parm[MEDBUF];
     int chid;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          print(1, 0, out, "Usage: part <chan> [:<reason>]");
          return;
     }

     if ((chid = remove_table(parm, channel, MAX_CHANS)))
     {
          print(1, 0, out, "%s: no such chan", parm);
          return;
     }

     free(chankey[chid]);
     chankey[chid] = NULL;

     clear_queue(&names[chid]);
     clear_queue(&names_op[chid]);

     print(1, 0, out, "leaving %s", parm);
     send2clones(buffer);
}

void parse_mode(char *buffer, int out)
{
     char parm[MEDBUF];
     int chid;
     clone_t *clone;

     if (StrParam(parm, sizeof(parm), buffer, 2))
     {
          if (!StrCmpPrefix(buffer, "mode"))
          {
               print(1, 0, out, "Usage: MODE <dest> <StrParams>");
               return;
          }
          if (!StrCmpPrefix(buffer, "kick"))
          {
               print(1, 0, out, "Usage: KICK <channel> <nick> [:<reason>]");
               return;
          }
          if (!StrCmpPrefix(buffer, "topic"))
          {
               print(1, 0, out, "Usage: TOPIC <channel> :<topic>");
               return;
          }
     }
     StrParam(parm, sizeof(parm), buffer, 1);
     if ((chid = getchid(parm)) == -1)
     {
          print(1, 0, out, "%s: no such chan", parm);
          return;
     }
     if (!(clone = getop(chid)))
     {
        print(1, 0, out, "Error: no ops in %s", parm);
        return;
     }
     send(clone->sock, buffer, strlen(buffer), 0);
}

void parse_privmsg(char *buffer, int out)
{
     char parm[MEDBUF];

     if (StrParam(parm, sizeof(parm), buffer, 2))
     {
          StrFirstToken(buffer);
          print(1, 0, out, "Usage: %s <dest> :<message>", buffer);
          return;
     }
     send2clones(buffer);
}

void parse_kb(char *buffer, int out)
{
     char parm[MEDBUF+1];
     clone_t *clone;

     if (StrParam(parm, sizeof(parm), buffer, 2))
     {
          print(1, 0, out, "Usage: %s <chan> <nick> [:<reason>]", KICKBAN);
          return;
     }

     StrParam(parm, sizeof(parm), buffer, 1);
     if ((mass_ch = getchid(parm)) == -1)
     {
          print(1, 0, out, "%s: no such chan", parm);
          return;
     }

     if (!(clone = getop(mass_ch)))
     {
          return;
     }

     free(mass_reas);
     if (sscanf(buffer, "%*s %*s %*s :%"MEDBUF_TXT"[^\n]", parm) != 1)
     {
          mass_reas = NULL;
     }
     else
     {
          mass_reas = StrDuplicate(parm);
     }

     if (!StrParam(parm, sizeof(parm), buffer, 2))
     {
          kickban(clone, parm);
          print(1, 0, out, "kickban %s", SETON);
     }
}

void parse_mkb(char *buffer, int out)
{
     char parm[MEDBUF+1];
     clone_t *clone;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          StrFirstToken(buffer);
          print(1, 0, out, "Usage: %s <chan> [:<reason>]", buffer);
          return;
     }

     if ((mass_ch = getchid(parm)) == -1)
     {
          print(1, 0, out, "%s: no such chan", parm);
          return;
     }

     if (!(clone = getop(mass_ch)))
          return;

     free(mass_reas);
     if (sscanf(buffer, "%*s %*s :%"MEDBUF_TXT"[^\n]", parm) != 1)
     {
          mass_reas = NULL;
     }
     else
     {
          mass_reas = StrDuplicate(parm);
     }

     StrFirstToken(buffer);
     print(1, 0, out, "%s %s on %s", buffer, SETON, channel[mass_ch]);

     mass_mode = !StrCmpPrefix(buffer, MASSKICKBAN) ? MKB : MK;
     init_massdo(mass_ch, mass_mode);
}

void parse_to(char *buffer, int out)
{
     char parm[MEDBUF];
     char chan[MINIBUF];
     clone_t *clone1;
     clone_t *clone2;

     if (StrParam(parm, sizeof(parm), buffer, 3))
     {
          StrFirstToken(buffer);
          print(1, 0, out, "Usage: %s <chan> <server1> <server2>", buffer);
          print(1, 0, out, "server1 = server to get op nicks");
          print(1, 0, out, "server2 = server to change nicks");
          return;
     }

     if (!(clone2 = getone(parm)))
     {
          print(1, 0, out, "no clone online on %s", parm);
          return;
     }

     StrParam(parm, sizeof(parm), buffer, 2);
     if (!(clone1 = getone(parm)))
     {
          print(1, 0, out, "no clone online on %s", parm);
          return;
     }

     StrParam(chan, sizeof(chan), buffer, 1);
     print(1, 0, out, "takeover activated on %s", chan);
     mass_mode = TO;
     mass_server = clone2->server;
     init_massdo(mass_ch, mass_mode);
}

void parse_mo(char *buffer, int out)
{
     char parm[MEDBUF];
     clone_t *clone;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          StrFirstToken(buffer);
          print(1, 0, out, "Usage: %s <chan>", buffer);
          return;
     }

     if ((mass_ch = getchid(parm)) == -1)
     {
          print(1, 0, out, "%s: no such chan", parm);
          return;
     }

     if (!(clone = getop(mass_ch)))
     {
          return;
     }

     StrFirstToken(buffer);
     print(1, 0, out, "%s %s on %s", buffer, SETON, channel[mass_ch]);

     if (!StrCmpPrefix(buffer, MASSOP))
     {
          force_massop();
          return;
     }

     if (!StrCmpPrefix(buffer, MASSDEOP))
     {
          mass_mode = MD;
          init_massdo(mass_ch, mass_mode);
          return;
     }

     send_sock(clone->sock, "MODE %s +b\n", channel[mass_ch]);
}

void parse_addprot(char *buffer, int out)
{
     char parm[MEDBUF];
     int i;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          print(1, 0, out, "Usage: %s <nick>", ADDPROT);
          return;
     }
     if (!occur_table(parm, conf.prot, MAX_PROTS))
     {
          if ((i = add_table(parm, conf.prot, MAX_PROTS)) != -1)
          {
               print(1, 0, out, "[%d] %s protected", i, parm);
          }
          else
          {
               print(1, 0, out, "list full");
          }
     }
}

void parse_rmprot(char *buffer, int out)
{
     char parm[MINIBUF];
     int i;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          print(1, 0, out, "Usage: %s <ID>", RMPROT);
          return;
     }
     if ((i = atoi(parm)) < 0)
     {
          print(1, 0, out, "protected nick list cleared");
          clear_table(conf.prot, MAX_PROTS);
          return;
     }
     if (i < MAX_PROTS && conf.prot[i])
     {
          print(1, 0, out, "%s not protected anymore", conf.prot[i]);
          free(conf.prot[i]);
          conf.prot[i] = NULL;
          return;
     }
     print(1, 0, out, "no such entry");
}

void parse_addop(char *buffer, int out)
{
     char parm[MEDBUF];
     int i;

     if (StrParam(parm, sizeof(parm), buffer, 1) || !is_pattern(parm))
     {
          print(1, 0, out, "Usage: %s <pattern>", ADDOP);
          return;
     }
     if ((i = add_table(parm, conf.aop, MAX_AOPS)) == -1)
     {
          print(1, 0, out, "aop list full");
          return;
     }
     print(1, 0, out, "[%d] %s added to the aop list", i, parm);
}

void parse_rmop(char *buffer, int out)
{
     char parm[MINIBUF];
     int i;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          print(1, 0, out, "Usage: %s <ID>", RMOP);
          return;
     }
     if ((i = atoi(parm)) < 0)
     {
          print(1, 0, out, "auto op list cleared");
          clear_table(conf.aop, MAX_AOPS);
          return;
     }
     if (i < MAX_AOPS && conf.aop[i])
     {
          print(1, 0, out, "%s removed from the aop list", conf.aop[i]);
          free(conf.aop[i]);
          conf.aop[i] = NULL;
          return;
     }
     print(1, 0, out, "no such entry");
}

void parse_addjupe(char *buffer, int out)
{
     char parm[MEDBUF];
     int i;
     int idx;
     int mon;
     unsigned int j;
     char cmdbuf[BIGBUF];
     clone_t **pcl;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          print(1, 0, out, "Usage: %s <nick>", ADDJUPE);
          return;
     }

     if ((idx = occur_table(parm, conf.jupe, MAX_JUPES)) != 0)
     {
          print(1, 0, out, "nick already juped as [%d]", (idx - 1));
          return;
     }

     for (i = mon = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if ((*pcl) && (*pcl)->online && (*pcl)->monitor_tmax != 0)
          {
               for (j = 0; j < (*pcl)->monitor_tmax; ++j) /* ``unsigned'' is not needed, stops a warning when compiled with `-Wsign-compare' */
               {
                    if ((*pcl)->jupes[j] == NULL)
                    {
                         if (idx != -1)
                         {
                              if((idx = add_table(parm, conf.jupe, MAX_JUPES)) == -1)
                              {
                                   print(1, 0, out, "jupe list full");
                                   return;
                              }
                              else
                              {
                                   print(1, 0, out, "[%d] %s added to the jupe list", idx, parm);
                                   idx = -1;
                              }
                         }

                         (*pcl)->jupes[j] = StrDuplicate(parm);
                         snprintf(cmdbuf, sizeof(cmdbuf), "MONITOR + %s\r\n", parm);
                         send((*pcl)->sock, cmdbuf, strlen(cmdbuf), 0);
                         print_prefix((*pcl), 0, 0);
                         print(1, 0, out, "%s added to my MONITOR list", parm);
                         mon++;
                         break;
                    }
               }
          }
     }

     if(mon == 0)
     {
          print(1, 0, out, "jupe list full (out of clones that could MONITOR)");
     }
}

void parse_rmjupe(char *buffer, int out)
{
     char parm[MINIBUF];
     int idx;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          print(1, 0, out, "Usage: %s <ID>", RMJUPE);
          return;
     }
     if ((idx = atoi(parm)) < 0)
     {
          clear_table(conf.jupe, MAX_JUPES);
          print(1, 0, out, "jupe list cleared");
          return;
     }
     if (idx < MAX_JUPES && conf.jupe[idx])
     {
          clone_t **pcl = cl;
          char *nick = conf.jupe[idx], cmdbuf[BIGBUF];
          int i;
          unsigned int j;

          print(1, 0, out, "%s removed from the jupe list", nick);
          for (i = 0; i < MAX_CLONES; ++i, ++pcl)
          {
               if ((*pcl) && (*pcl)->online && (*pcl)->monitor_tmax != 0)
               {
                    for (j = 0; j < (*pcl)->monitor_tmax; ++j) /* `unsigned' is not needed, stops a warning when compiled with `-Wsign-compare' */
                    {
                         if (0 == StrCompare((*pcl)->jupes[j], nick))
                         {
                              snprintf(cmdbuf, sizeof(cmdbuf), "MONITOR - %s\r\n", nick);
                              send((*pcl)->sock, cmdbuf, strlen(cmdbuf), 0);
                              (*pcl)->jupes[j] = NULL;
                              print_prefix((*pcl), 0, 0);
                              print(1, 0, out, "%s removed from my MONITOR list", nick);
                              break;
                         }
                    }
               }
               if ((*pcl) && (*pcl)->online && (*pcl)->grabbing && (0 == StrCompare((*pcl)->nick, nick)))
               {
                    (*pcl)->grabbing = 0;
                    print_prefix((*pcl), 0, 0);
                    print(1, 0, out, "I ungrabbed `%s'", nick);
               }
          }

          free(conf.jupe[idx]);
          conf.jupe[idx] = NULL;
          return;
     }

     print(1, 0, out, "no such entry");
}

void parse_addshit(char *buffer, int out)
{
     char parm[MEDBUF+1];
     int i;

     if (sscanf(buffer, "%*s %"MEDBUF_TXT"[^\n]", parm) != 1 || !is_pattern(parm))
     {
          print(1, 0, out, "Usage: %s <pattern> :[<reason>]", ADDSHIT);
          return;
     }
     if ((i = add_table(parm, conf.shit, MAX_SHITS)) == -1)
     {
          print(1, 0, out, "shitlist full");
          return;
     }
     StrFirstToken(parm);
     print(1, 0, out, "[%d] %s shitlisted", i, parm);
}

void parse_rmshit(char *buffer, int out)
{
     char parm[MINIBUF];
     int i;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          print(1, 0, out, "Usage: %s <ID>", RMSHIT);
          return;
     }
     if ((i = atoi(parm)) < 0)
     {
          print(1, 0, out, "shitlist cleared");
          clear_table(conf.shit, MAX_SHITS);
          return;
     }
     if (i < MAX_SHITS && conf.shit[i])
     {
          StrFirstToken(conf.shit[i]);
          print(1, 0, out, "%s removed from the shitlist", conf.shit[i]);
          free(conf.shit[i]);
          conf.shit[i] = NULL;
          return;
     }
     print(1, 0, out, "no such entry");
}

void parse_addscan(char *buffer, int out)
{
     char parm[MINIBUF];
     char save[MINIBUF];
     char server[MINIBUF];
     int type;
     int mode;
     int proxy_port;
     int server_port;
     clone_t *clone;

     if (StrParam(save, sizeof(save), buffer, 5))
     {
          print(1, 0, out, "Usage: %s <type> <scan port> <server> <port> <filename> [<mode>]", ADDSCAN);
          return;
     }
     if (!(clone = getscan()))
     {
          print(1, 0, out, "scan: no clone available");
          return;
     }
     StrParam(server, sizeof(server), buffer, 3);
     if (sscanf(buffer, "%*s %d", &type) != 1 || type < 0 || type > 6)
     {
          print(1, 0, out, "scan: invalid type");
          return;
     }
     if ((sscanf(buffer, "%*s %*s %d", &proxy_port) != 1) || sscanf(buffer, "%*s %*s %*s %*s %d", &server_port) != 1)
     {
          print(1, 0, out, "scan: invalid port");
          return;
     }
     if (!StrParam(parm, sizeof(parm), buffer, 6))
     {
          mode = atoi(parm);
     }
     else
     {
          mode = M_QUIT;
     }
     if (mode < 0 || mode > 2)
     {
          print(1, 0, out, "scan: invalid mode");
          return;
     }
     clone->scan = (scan_t *)xmalloc(sizeof(scan_t));
     clone->scan->type = type;
     clone->scan->proxy_port = proxy_port;
     clone->scan->server = StrDuplicate(server);
     clone->scan->server_port = server_port;
     clone->scan->save = StrDuplicate(save);
     clone->scan->mode = mode;
     print(1, 0, out, "[%d] scan activated", clone->id);
}

void parse_rmscan(char *buffer, int out)
{
     char parm[MEDBUF];
     int i;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          print(1, 0, out, "Usage: %s <ID>", RMSCAN);
          return;
     }
     if (sscanf(parm, "%d", &i) == 1)
     {
          if (i < MAX_CLONES && cl[i] && cl[i]->scan)
          {
               print(1, 0, out, "[%d] scan canceled", i);
               free(cl[i]->scan->server);
               free(cl[i]->scan->save);
               free(cl[i]->scan);
               cl[i]->scan = NULL;
               return;
          }
     }
     print(1, 0, out, "no such entry");
}

void parse_echo(char *buffer, int out)
{
     char parm[MEDBUF];

     free(target);
     if (!StrParam(parm, sizeof(parm), buffer, 1))
     {
          target = StrDuplicate(parm);
          print(1, 0, out, "echo enabled, echoing `%s'", target);
     }
     else
     {
          target = NULL;
          print(1, 0, out, "echo disabled");
     }
}

void parse_chankey(char *buffer, int out)
{
     char parm[MEDBUF];
     int i;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          print(1, 0, out, "Usage: %s <chan> <key>", CHANKEY);
          return;
     }
     if ((i = getchid(parm)) == -1)
     {
          print(1, 0, out, "%s: no such chan", parm);
          return;
     }
     free(chankey[i]);
     if (!StrParam(parm, sizeof(parm), buffer, 2))
     {
          chankey[i] = StrDuplicate(parm);
          print(1, 0, out, "Using chankey %s on %s", chankey[i], channel[i]);
     }
     else
     {
          chankey[i] = NULL;
          print(1, 0, out, "Chankey disabled on %s", channel[i]);
     }
}

void parse_nicklist(char *buffer, int out)
{
     char parm[MEDBUF];
     FILE *f;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          print(1, 0, out, "Usage: %s <filename>", NICKLIST);
          return;
     }

     if (!(f = fopen(parm, "r")))
     {
          print(1, 0, out, "Cannot open the file `%s'", parm);
          return;
     }

     clear_table(conf.nicks, MAX_NICKS);
     fill_table(conf.nicks, MAX_NICKS, f);
     fclose(f);

     print(1, 0, out, "nicklist loaded", parm);
}

void parse_select(char *buffer, int out)
{
     clone_t **pcl;
     register int i;
     char parm[MINIBUF];

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {
          print(1, 0, out, "Usage: %s <nick> :<IRC command>", SELECT);
          return;
     }

     strsep(&buffer, ":");
     if (!buffer)
     {
          return;
     }

     if (parm[0] == '*')
     {
          send2clones(buffer);
          return;
     }

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if ((*pcl) && (*pcl)->online && !StrCompare((*pcl)->nick, parm))
          {
               send((*pcl)->sock, buffer, strlen(buffer), 0);
               return;
          }
     }
     print(1, 0, out, "no such nickname");
}

void parse_load(char *buffer, int out)
{
     int type;
     int proxy_port;
     int server_port;
     char parm[MINIBUF];
     char server[MINIBUF];
     char proxy[MINIBUF];

     if (StrParam(parm, sizeof(parm), buffer, 5))
     {
          print(1, 0, out, "Usage: %s <type> <proxy> <port> <server> <port>", LOAD);
          return;
     }
     if (sscanf(buffer, "%*s %d", &type) != 1 || type < 0 || type > 5)
     {
          print(1, 0, out, "load: invalid type");
          return;
     }
     if (sscanf(buffer, "%*s %*s %*s %d", &proxy_port) != 1 || sscanf(buffer, "%*s %*s %*s %*s %*s %d", &server_port) != 1)
     {
          print(1, 0, out, "load: invalid port");
          return;
     }
     StrParam(server, sizeof(server), buffer, 4);
     StrParam(proxy, sizeof(proxy), buffer, 2);
     load_host(type, proxy, proxy_port, server, server_port, NULL, NULL, NULL, M_NORMAL);
}

void parse_quit(char *buffer, int out)
{
     if (!StrCmpPrefix(buffer, "quit :"))
     {
          send2clones(buffer);
          print(1, 0, out, "Exiting...");
          sleep(1);
     }

     main_exit();
}
/*
void parse_maxconns(char *buffer, int out)
{
     char parm[MINIBUF];
     int i;

     if (StrParam(parm, sizeof(parm), buffer, 1))
     {   
          print(1, 0, out, "Usage: %s <max_conns>", MAXCONNS);
          return;
     }
     else if(i = atoi(parm) > 0)
     {
          print(1, 0, out, "MAX_CONNS set to %d", i);
          conf.max_conns = i;
          return;
     }
}
*/
/* }}} */
/* {{{ interpret() */
void interpret(char *buffer, int out)
{
     static cmd_t cmd[] =
          { { "join",        parse_join              },
            { "part",        parse_part              },
            { SELECT,        parse_select            },
            { LOAD,          parse_load              },
            { MUTE,          parse_mute              },
            { "mode",        parse_mode              },
            { "kick",        parse_mode              },
            { "topic",       parse_mode              },
            { "privmsg",     parse_privmsg           },
            { "notice",      parse_privmsg           },
            { KICKBAN,       parse_kb                },
            { MASSKICKBAN,   parse_mkb               },
            { MASSKICK,      parse_mkb               },
            { MASSOP,        parse_mo                },
            { MASSDEOP,      parse_mo                },
            { MASSUNBAN,     parse_mo                },
            { TAKEOVER,      parse_to                },
            { ADDSCAN,       parse_addscan           },
            { RMSCAN,        parse_rmscan            },
            { ADDPROT,       parse_addprot           },
            { RMPROT,        parse_rmprot            },
            { ADDOP,         parse_addop             },
            { RMOP,          parse_rmop              },
            { ADDJUPE,       parse_addjupe           },
            { RMJUPE,        parse_rmjupe            },
            { ADDSHIT,       parse_addshit           },
            { RMSHIT,        parse_rmshit            },
            { ECHO,          parse_echo              },
            { CHANKEY,       parse_chankey           },
            { NICKLIST,      parse_nicklist          },
/*            { MAXCONNS,      parse_maxconns          }, */
            { "quit",        parse_quit              },
            { NULL,          NULL                    }
          };
     cmd_t *pcmd;

     for (pcmd = cmd; pcmd->parm; ++pcmd)
     {
          if (!StrCmpPrefix(buffer, pcmd->parm))
          {
               pcmd->function(buffer, out);
               return;
          }
     }

     if (out > 0 && (!StrCmpPrefix(buffer, "help") || buffer[0] == '?'))
     {
          usage(out);
     }
     else if (conf.nicks[0] && !StrCmpPrefix(buffer, RANDOM))
     {
          conf.use_wordlist = !conf.use_wordlist;
          print(1, 0, out, "using %s", conf.use_wordlist ? "the nicks" : "random nicks");
     }
     else if (!StrCmpPrefix(buffer, AGGRESS))
     {
          conf.aggressive = !conf.aggressive;
          print(1, 0, out, "aggressive mode set %s", conf.aggressive ? "ON" : "OFF");
     }
     else if (!StrCmpPrefix(buffer, PEACE))
     {
          conf.peace = !conf.peace;
          print(1, 0, out, "peace mode set %s", conf.peace ? "ON" : "OFF");
     }
     else if (out > 0 && !StrCmpPrefix(buffer, STATUS))
     {
          status(out);
     }
     else if (!StrCmpPrefix(buffer, NICKS))
     {
          send2clones(buffer);
     }
     else if (buffer[0] != '\n')
     {
          print(1, 0, out, "muhstik: %s: command not found", strsep(&buffer, DELIM));
     }
}
/* }}} */
/* }}} */
