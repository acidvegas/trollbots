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
#include <stdarg.h>
#include <time.h>

#include "../include/print.h"
#include "../include/string.h"
#include "../include/lists.h"
#include "../include/clone.h"
/* }}} */
/* {{{ Variables */
/* {{{ External Variables */
extern config_t conf;
extern int mute;
extern char *channel[];
extern char *broth[];
extern clone_t *cl[];
extern const char *strtype[];
extern char *op_nick;
extern int t0;
extern pthread_mutex_t mutex[];
/* }}} */
/* }}} */
/* {{{ print_*() */
inline void print_error(char *prefix)
{
     print(1, 0, 0, "%s: %s", prefix, strerror(errno));
}

void print_irc(char *buffer, int color, int sock)
{
     static char *color_codes[] = { "", "\026", "\0032", "\0037", "\0035", "\00314" };

     send(sock, color_codes[color], strlen(color_codes[color]), 0);
     send(sock, buffer, strlen(buffer), 0);
}

void print_privmsg(char *buffer, int color, int dest)
{
     char tosend[BIGBUF];
     dest = -dest-2;
     snprintf(tosend, sizeof(tosend), "PRIVMSG %s :%s", op_nick, buffer);
     print_irc(tosend, color, cl[dest]->sock);
}

void print_console(char *buffer, int ret, int color, int dest)
{
     int len;
     static char *color_term[] = { "\033[0;0m", "\033[0;35m", "\033[0;34m", "\033[0;1;1m", "\033[0;31m", "" };

     if (!conf.nocolor)
     {
          len = strlen(buffer) - 1;
          if (buffer[len] == '\n')
          {
              ret = 1;
          }
          if (ret)
          {
              buffer[len] = 0;
          }
          printf("%s", color_term[color]);
     }

     printf("%s", buffer);

     if (!conf.nocolor)
     {
          printf("%s", color_term[0]);
          if (ret)
          {
              printf("\n");
          }
     }
}

void print(int ret, int color, int dest, const char *fmt, ...)
{
     char buffer[BIGBUF];
     va_list ap;

     if (dest == -1)
     {
          return;
     }

     va_start(ap, fmt);
     vsnprintf(buffer, sizeof(buffer), fmt, ap);
     va_end(ap);

     if (ret)
     {
          StrCat(buffer, "\n", sizeof(buffer));
     }

     if (dest <= -2)
     {
          print_privmsg(buffer, color, dest);
          return;
     }

     if (!mute)
     {
          pthread_mutex_lock(&mutex[1]);
          print_console(buffer, ret, color, dest);
          pthread_mutex_unlock(&mutex[1]);
     }
}

void print_prefix(clone_t *clone, int color, int dest)
{
     print(0, color, dest, "[%s;%s;%s] ", clone->nick, clone->proxy ? clone->proxy : "", clone->server);
}

void print_desc(int out, char *com, char *desc)
{
     register int i;

     print(0, 0, out, "%s", com);
     for (i = strlen(com); i < LEN_TAB; ++i)
     {
          print(0, 0, out, " ");
     }
     print(1, 0, out, "%s", desc);
}

void print_line(int out)
{
     register int i;

     if (out >= 0)
     {
          for (i = 0; i < LEN_LINE; ++i)
          {
               print(0, 0, out, "-");
          }
     }
     print(0, 0, out, "\n");
}

void print_motd(int out)
{
     FILE *f;
     char buffer[BIGBUF];

     if ((f = fopen(conf.motd, "r")))
     {
          while (fgets(buffer, sizeof(buffer), f))
          {
               print(0, 0, out, "%s", buffer);
          }
          fclose(f);
     }
}
/* }}} */
/* {{{ usage() */
void usage(int out)
{
     char *(*p)[2];
     static char *desc_general[][2] =
          {
               { "help or ?", "print this help"                                      },
               { STATUS,      "print status {uptime,nicks,channels,users}"           },
               { NULL,        NULL                                                   }
          };
     static char *desc_actions[][2] =
          {
               { "join",      "JOIN or reJOIN a channel"                             },
               { "part",      "PART a channel"                                       },
               { "quit",      "QUIT IRC and shut down the bot"                       },
               { "privmsg",   "send a message or a CTCP to a nick or a channel"      },
               { NICKS,       "change all clone nicks"                               },
               { KICKBAN,     "kickban a nick from a channel"                        },
               { ECHO,        "make all the clones repeat what one nick says"        },
               { TAKEOVER,    "collide nicks on different servers"                   },
               { SELECT,      "make one clone send something to IRC"                 },
               { NULL,        NULL                                                   }
          };
     static char *desc_modes[][2] =
          {
               { AGGRESS,     "deop enemys actively and kick them on privmsg"        },
               { PEACE,       "don't automatically deop enemies"                     },
               { RANDOM,      "use the wordlist to set the nicks"                    },
               { MUTE,        "stop writing to stdout"                               },
               { NULL,        NULL                                                   }
          };
     static char *desc_env[][2] =
          {
               { NICKLIST,    "change the active wordlist"                           },
               { CHANKEY,     "set a key to be used when reJOINing a +k channel"     },
               { LOAD,        "load a clone dynamically"                             },
               { ADDPROT "/" RMPROT, "set/unset a protected nick"                    },
               { ADDJUPE "/" RMJUPE, "add/remove a nick to/from the jupe list"       },
               { ADDOP   "/" RMOP,   "add/remove a pattern to/from the aop list"     },
               { ADDSHIT "/" RMSHIT, "add/remove a pattern to/from the shitlist"     },
               { ADDSCAN "/" RMSCAN, "set/unset a scan on JOIN"                      },
               { NULL,        NULL                                                   }
          };

     print(1, 0, out, "Available commands:");
     print_line(out);
     for (p = desc_general; **p; ++p)
     {
          print_desc(out, (*p)[0], (*p)[1]);
     }

     print(1, 0, out, "\n- Actions:");
     for (p = desc_actions; **p; ++p)
     {
          print_desc(out, (*p)[0], (*p)[1]);
     }

     print(1, 0, out, "%s, %s, %s, %s, %s: mass{op,KICK,KICKban,deop,unban}", MASSOP, MASSKICK, MASSKICKBAN, MASSDEOP, MASSUNBAN);
     print(1, 0, out, "{KICK,MODE,TOPIC}: IRC protocol");

     print(1, 0, out, "\n- Modes:");
     for (p = desc_modes; **p; ++p)
     {
          print_desc(out, (*p)[0], (*p)[1]);
     }

     print(1, 0, out, "\n- Environment:");
     for (p = desc_env; **p; ++p)
     {
          print_desc(out, (*p)[0], (*p)[1]);
     }

     print_line(out);
}
/* }}} */
/* {{{ print_uptime() */
void print_uptime(int out)
{
     int uptime;
     uptime = time(NULL) - t0;
     print(1, 0, out, "muhstik v. %s, uptime: %d days, %02d:%02d:%02d", VERSION, uptime/86400, (uptime/3600)%24, (uptime/60)%60, uptime%60);
}
/* }}} */
/* {{{ nofclones() */
int nofclones()
{
     register int i;
     int k = 0;
     clone_t **pcl;

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if (*pcl && (*pcl)->online)
          {
               ++k;
          }
     }
     for (i = 0; i < MAX_BROTHERS; ++i)
     {
          if (broth[i])
          {
               ++k;
          }
     }
     return k;
}
/* }}} */
/* {{{ is_empty_*() booleans */
int is_empty_str(char **list, int max)
{
     register int i;

     for (i = 0; i < max; ++i, ++list)
     {
          if (*list)
          {
               return 0;
          }
     }
     return 1;
}

int is_empty_scan(clone_t **list, int max)
{
     register int i;
     for (i = 0; i < max; ++i, ++list)
     {
          if (*list && (*list)->scan)
          {
               return 0;
          }
     }
     return 1;
}
/* }}} */
/* {{{ print_nicks() */
void print_nicks(int out)
{
     char nicks[BIGBUF];
     char **p;
     clone_t **pcl;
     register int i;

     if (!(i = nofclones()))
     {
          print(1, 0, out, "- no clones online");
          return;
     }

     print(1, 0, out, "- %d clones online:", i);

     memset(nicks, 0, sizeof(nicks));

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if (*pcl && (*pcl)->online)
          {
               StrCat(nicks, (*pcl)->nick, sizeof(nicks));
               StrCat(nicks, " ", sizeof(nicks));
          }
     }

     for (i = 0, p = broth; i < MAX_BROTHERS; ++i, ++p)
     {
          if (*p)
          {
               StrCat(nicks, *p, sizeof(nicks));
               StrCat(nicks, " ", sizeof(nicks));
          }
     }

     print(1, 0, out, "%s", nicks);
}
/* }}} */
/* {{{ nops() */
int nops(int chid)
{
     register int i;
     clone_t **pcl;
     int k = 0;

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if (is_op(*pcl, chid))
          {
               ++k;
          }
     }

     return k;
}
/* }}} */
/* {{{ print_*() */
void print_channels(int out)
{
     char **p;
     register int i;

     print(1, 0, out, "- channels:");
     for (i = 0, p = channel; i < MAX_CHANS; ++i, ++p)
     {
          if (*p)
          {
               print(1, 0, out, "%s (%d opped) ", *p, nops(i));
          }
     }
}

void print_scans(int out)
{
     clone_t **p;
     register int i;

     print(1, 0, out, "- scans:");
     for (i = 0, p = cl; i < MAX_CLONES; ++i, ++p)
     {
          if ((*p) && (*p)->online && (*p)->scan)
          {
               print(1, 0, out, "[%d] nick=%s, type=%s, port=%d, server=%s, saveto=%s",
                     i, (*p)->nick, strtype[(*p)->scan->type], (*p)->scan->proxy_port, (*p)->scan->server, (*p)->scan->save);
          }
     }
}

void print_aops(int out)
{
     char **p;
     register int i;

     print(1, 0, out, "- auto op list:");
     for (i = 0, p = conf.aop; i < MAX_AOPS; ++i, ++p)
     {
          if (*p)
          {
               print(1, 0, out, "[%d] %s", i, *p);
          }
     }
}
void print_jupes(int out)
{
     char **p;
     register int i;

     print(1, 0, out, "- nick jupe list:");
     for (i = 0, p = conf.jupe; i < MAX_JUPES; ++i, ++p)
     {
          if (*p)
          {
               print(1, 0, out, "[%d] %s", i, *p);
          }
     }
}

void print_prot(int out)
{
     char **p;
     register int i;

     print(1, 0, out, "- protected nicks:");
     for (i = 0, p = conf.prot; i < MAX_PROTS; ++i, ++p)
     {
          if (*p)
          {
               print(1, 0, out, "[%d] %s", i, *p);
          }
     }
}

void print_shit(int out)
{
     char **p;
     register int i;

     print(1, 0, out, "- shitlist:");
     for (i = 0, p = conf.shit; i < MAX_SHITS; ++i, ++p)
     {
          if (*p)
          {
               if (strchr(*p, ':'))
               {
                    print(1, 0, out, "[%d] %s", i, *p);
               }
               else
               {
                    print(1, 0, out, "[%d] %s :no reason", i, *p);
               }
          }
     }
}
/* }}} */
/* {{{ status() */
void status(int out)
{
     print_line(out);
     print_uptime(out);
     print_nicks(out);

     if (!is_empty_str(channel, MAX_CHANS))
     {
          print_channels(out);
     }

     if (!is_empty_scan(cl, MAX_CLONES))
     {
          print_scans(out);
     }

     if (!is_empty_str(conf.aop, MAX_AOPS))
     {
          print_aops(out);
     }

     if (!is_empty_str(conf.jupe, MAX_JUPES))
     {
          print_jupes(out);
     }

     if (!is_empty_str(conf.prot, MAX_PROTS))
     {
          print_prot(out);
     }

     if (!is_empty_str(conf.shit, MAX_SHITS))
     {
          print_shit(out);
     }

     print_line(out);
}
/* }}} */
