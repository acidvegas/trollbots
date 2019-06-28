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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "../include/mass.h"
#include "../include/control.h"
#include "../include/net.h"
#include "../include/string.h"
#include "../include/lists.h"
/* }}} */
/* {{{ Variables */
/* {{{ External Variables */
extern char *channel[];
extern char *broth[];
extern clone_t *cl[];
extern config_t conf;
extern queue names_op[];
extern queue names[];
/* }}} */
/*{{{ Global variables */
int mass_ch;
int mass_mode;
char *mass_reas;
char *mass_server;
/* }}} */
/* }}} */
/* {{{ Massmode functions */
/* {{{ multimode() */
void multimode(clone_t *clone, char *chan, char *sign, int n, char *nicks, int uniq)
{
     char modes[MINIBUF];
     int i;

     memset(modes, 0, sizeof(modes));

     StrCat(modes, " ", sizeof(modes));
     StrCat(modes, sign, sizeof(modes));
     for (i = 0; i < n; ++i)
     {
          StrCat(modes, "o", sizeof(modes));
     }

     if (uniq)
     {
          send2server(clone, "MODE %s%s%s\n", chan, modes, nicks);
     }
     else
     {
          send_sock(clone->sock, "MODE %s%s%s\n", chan, modes, nicks);
     }
}
/* }}} */
/* {{{ (de)opping and kicking functions */
/* TODO: Deop +{h,a,q}, if possible. */
void massdeop(clone_t *clone, int chid)
{
     int i;
     int n = 0;
     char nicks[BIGBUF];
     queue *list;

     memset(nicks, 0, sizeof(nicks));
     list = &names_op[chid];

     for (i = 0; *list && i < conf.multi_deop; ++i)
     {
          if (is_enemy((*list)->data))
          {
               StrCat(nicks, " ", sizeof(nicks));
               StrCat(nicks, (*list)->data, sizeof(nicks));
               ++n;
          }
          rotate_cell(list);
     }

     if (n > 0)
     {
          multimode(clone, channel[chid], "-", n, nicks, 1);
     }
}

void massop(clone_t *clone, int chid)
{
     register int i;
     register int j;
     clone_t **pcl;
     char nicks[BIGBUF];

     memset(nicks, 0, sizeof(nicks));

     for (i = j = 0, pcl = cl; j < conf.multi_op && i < MAX_CLONES; ++i, ++pcl)
     {
          if ((*pcl) && (*pcl) != clone && (*pcl)->online && (*pcl)->needop[chid])
          {
               (*pcl)->needop[chid] = 0;
               StrCat(nicks, " ", sizeof(nicks));
               StrCat(nicks, (*pcl)->nick, sizeof(nicks));
               ++j;
          }
     }

     if (j > 0)
     {
          multimode(clone, channel[chid], "+", j, nicks, 0);
     }
}

void force_massop()
{
     register int i;
     char **pt;
     clone_t **pcl;
     clone_t *one;

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if ((*pcl) && (*pcl)->online && !(*pcl)->restricted && !(*pcl)->op[mass_ch] && (one = getop(mass_ch)))
          {
               op(one, mass_ch, (*pcl)->nick);
          }
     }

     for (i = 0, pt = broth; i < MAX_BROTHERS; ++i, ++pt)
     {
          if ((*pt) && (one = getop(mass_ch)))
          {
               op(one, mass_ch, *pt);
          }
     }
}

void _masskick(clone_t *clone, int chid, queue **list)
{
     int n;
     char buffer[BIGBUF];

     memset(buffer, 0, sizeof(buffer));

     for (n = 0; **list && n < conf.multi_kick; ++n)
     {
          if (n > 0)
          {
               StrCat(buffer, ",", BIGBUF);
          }
          StrCat(buffer, (**list)->data, BIGBUF);
          free_cell(*list);
     }

     kick(clone, chid, buffer, mass_reas ? mass_reas : NULL, 0);
}

void _massdeop(clone_t *clone, int chid, queue **list)
{
     int i;
     char nicks[BIGBUF];

     memset(nicks, 0, sizeof(nicks));

     for (i = 0; **list && i < conf.multi_deop; ++i)
     {
          StrCat(nicks, " ", sizeof(nicks));
          StrCat(nicks, (**list)->data, sizeof(nicks));
          free_cell(*list);
     }

     if (i > 0)
     {
          multimode(clone, channel[chid], "-", i, nicks, 0);
     }
}
/* }}} */
/* {{{ massdo()ing functions */
void massdo(queue *list, int chid, int mode)
{
     int id;
     clone_t **pcl;
     int n;

     for (n = 0; *list && n < 10; ++n)
     {
          for (id = 0, pcl = cl; *list && id < MAX_CLONES; ++id, ++pcl)
          {
               if (is_op(*pcl, chid))
               {
                    switch (mode)
                    {
                         case MK:
                              _masskick(*pcl, chid, &list);
                              break;
                         case MD:
                              _massdeop(*pcl, chid, &list);
                              break;
                         case MKB:
                              kickban(*pcl, (*list)->data);
                              free_cell(list);
                              break;
                    }
                    (*pcl)->lastsend = time(NULL);
               }
          } /* bracket might be after the `usleep()' -- Leon */
          usleep(500000);
     }
}

void takeover(queue *list)
{
     register int i;
     clone_t **pcl;

     for (i = 0, pcl = cl; *list && i < MAX_CLONES; ++i, ++pcl)
     {
          if ((*pcl) && (*pcl)->online && (*pcl)->server == mass_server)
          {
               send_irc_nick(*pcl, (*list)->data);
               free_cell(list);
          }
     }
}

void *init_massdo(int chid, int mode)
{
     queue list1 = NULL;
     queue list2 = NULL;
     queue plist;

     for (plist = names_op[chid]; plist; plist = plist->next)
     {
          if (mode == TO || is_enemy(plist->data))
          {
               add_queue(plist->data, &list1);
          }
     }
     for (plist = names[chid]; plist; plist = plist->next)
     {
          if (mode != TO && mode != MD && is_enemy(plist->data))
          {
               add_queue(plist->data, &list2);
          }
     }

     switch (mode)
     {
          case MKB:
          case MK:
               massdo(&list1, chid, mode);
               massdo(&list2, chid, mode);
               break;
          case MD:
               massdo(&list1, chid, mode);
               break;
          case TO:
               takeover(&list1);
               break;
     }

     clear_queue(&list1);
     clear_queue(&list2);

     return NULL;
}
/* }}} */
/* }}} */
