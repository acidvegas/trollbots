/* Muhstik, Copyright (C) 2001-2002, Louis Bavoil <mulder@gmx.fr>       */
/*                        2003, roadr (bigmac@home.sirklabs.hu)         */
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
#include <stdarg.h>
#include <unistd.h>
#include <time.h>

#include "../include/globals.h"
#include "../include/load.h"
#include "../include/control.h"
#include "../include/print.h"
#include "../include/proxy.h"
#include "../include/string.h"
#include "../include/lists.h"
#include "../include/net.h"
#include "../include/mass.h"
#include "../include/muhstik.h"
/* }}} */
/* {{{ Variables */
/* {{{ External variables */
extern config_t conf;
extern char *hostname;
extern char *channel[];
extern char *chankey[];
extern int mass_ch;
extern char *mass_reas;
extern char *target;
extern int echo_mode;
extern const char *strtype[];
/* }}} */
/*{{{ Global variables */
char *op_nick;
clone_t *cl[MAX_CLONES];
char *broth[MAX_BROTHERS];
queue public_queue;
queue names_op[MAX_CHANS];
queue names[MAX_CHANS];
time_t lastlock;
/* }}} */
/* }}} */
/*{{{ Function prototypes */
/* {{{ void do_jupe(clone_t *clone, char *nick); */
/**
 * do_jupe()
 * Function prototype for `do_jupe()'.
 */
void do_jupe(clone_t *clone, char *nick);
/* }}} */
/* }}} */
/* {{{ free()ing and closing functions */
/* {{{ void close_sock(clone_t *clone) */
void close_sock(clone_t *clone)
{
     if (clone->sock != -1)
     {
          close(clone->sock); /* XXX: free()? -- Leon */
     }
}
/* }}} */
/* {{{ void free_clone(clone_t *clone) */
void free_clone(clone_t *clone)
{
     int id = clone->id;

     close_sock(clone);

     /* Free the clone memory */
     free(clone->jupes);
     free(clone->nick);
     free(clone->ident);
     free(clone->real);
     free(clone->proxy);
     free(clone->server);
     free(clone->server_pass);
     free(clone->server_ident);
     free(clone->save);
     clear_queue(&clone->queue);
     free(clone);

     /* Free its slot */
     cl[id] = NULL;
}
/* }}} */
/* }}} */
/* {{{ faux boolean functions */
/* {{{ int not_a_clone(char *s) */
/**
 * not_a_clone()
 * @return boolean value representing whether or not the nick is a clone
 *          controlled by this muhstik iteration.
 */
int not_a_clone(char *s)
{
     register int i;
     clone_t **pcl;
     char **pt;

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if (*pcl && !StrCompare((*pcl)->nick, s))
          {
               return 0;
          }
     }

     for (i = 0, pt = broth; i < MAX_BROTHERS; ++i, ++pt)
     {
          if (*pt && !StrCompare(*pt, s))
          {
               return 0;
          }
     }

     return 1;
}
/* }}} */
/* {{{ int not_a_mast(char *s) */
int not_a_mast(char *s)
{
     register int i;
     char **pm;

     for (i = 0, pm = conf.prot; i < MAX_PROTS; ++i, ++pm)
     {
          if (*pm && !StrCompare(*pm, s))
          {
               return 0;
          }
     }

     return 1;
}
/* }}} */
/* {{{ int is_enemy(char *s) */
/**
 * is_enemy()
 * @return boolean value representing whether or not the nickname `s' is neither
 *          a clone nor a master.
 */
int is_enemy(char *s)
{
     return (not_a_clone(s) && not_a_mast(s));
}
/* }}} */
/* {{{ int is_op(clone_t *clone, int chid) */
/**
 * is_op()
 * @return boolean value representing whether or not `clone' is an op in channel
 *          `chid'.
 */
int is_op(clone_t *clone, int chid)
{
     return (clone && clone->online && clone->op[chid]);
}
/* }}} */
/* }}} */
/* {{{ Functions for opped clones */
/* {{{ void op(clone_t *clone, int chid, char *s) */
/**
 * op()
 * Op the nick `s' in the channel with channelid `chid' assigned to it.
 */
void op(clone_t *clone, int chid, char *s)
{
     send2server(clone, "MODE %s +o %s\n", channel[chid], s);
}
/* }}} */
/* {{{ void deop(clone_t *clone, int chid, char *nick) */
/**
 * deop()
 * Deop the nick `s' in the channel with channelid `chid' assigned to it.
 */
void deop(clone_t *clone, int chid, char *nick)
{
     send2server(clone, "MODE %s -o %s\n", channel[chid], nick);
}
/* }}} */
/* {{{ int deop_enemy(clone_t *clone, int chid, char *nick) */
/**
 * deop_enemy()
 * Deop enemy nick `nick' in channel with channelid `chid' assigned to it.
 * @return boolean indicating success of the deopping.
 */
int deop_enemy(clone_t *clone, int chid, char *nick)
{
     /* Don't try to deop a server */
     if (!nick[0])
     {
          return 1;
     }

     if (is_enemy(nick))
     {
          deop(clone, chid, nick);
          return 1;
     }

     return 0;
}
/* }}} */
/* {{{ void kick(clone_t *clone, int chid, char *s, char *reas, int mode) */
/**
 * kick()
 * Kick the nick `s' from the channel with channelid `chid' with reason `reas'.
 */
void kick(clone_t *clone, int chid, char *s, char *reas, int mode)
{
     char tmp[BIGBUF];

     snprintf(tmp, sizeof(tmp), "KICK %s %s :%s\n", channel[chid], s, reas ? reas : KICK_REAS);
     if (!mode)
     {
          send_sock(clone->sock, "%s", tmp);
     }
     else
     {
          send2server(clone, "%s", tmp);
     }
}
/* }}} */
/* {{{ void ban(clone_t *clone, int chid, char *s, int mode) */
/**
 * ban()
 * Ban host `s' from channel with channelid `chid' assigned to it.
 */
void ban(clone_t *clone, int chid, char *s, int mode)
{
     char tmp[BIGBUF];

     sscanf(s, "%*[^@]@%s", s);
     snprintf(tmp, sizeof(tmp), "MODE %s +b *!*@%s\n", channel[chid], s);

     if (!mode)
     {
          send_sock(clone->sock, "%s", tmp);
     }
     else
     {
          send2server(clone, "%s", tmp);
     }
}
/* }}} */
/* {{{ void kickban(clone_t *clone, char *s) */
void kickban(clone_t *clone, char *s)
{
     send_sock(clone->sock, "WHOIS %s\n", s);
     clone->wait_whois = 1;
}
/* }}} */
/* {{{ void unban(clone_t *clone, int chid, char *s) */
/**
 * unban()
 * Unban host `s' from the channel with channelid `chid' assigned to it.
 */
void unban(clone_t *clone, int chid, char *s)
{
     send_sock(clone->sock, "MODE %s -b %s\n", channel[chid], s);
}
/* }}} */
/* }}} */
/* {{{ Misc clone->IRC communication functions */
/* {{{ void join(clone_t *clone, char *dest) */
/**
 * join()
 * Join channel `dest'.
 */
void join(clone_t *clone, char *dest)
{
     int chid;

     if ((chid = getchid(dest)) == -1)
     {
          return;
     }

     clone->op[chid] = 0;
     clone->needop[chid] = 0;

     if (chankey[chid] == NULL)
     {
          send_sock(clone->sock, "JOIN %s\n", dest);
     }
     else
     {
          send_sock(clone->sock, "JOIN %s :%s\n", dest, chankey[chid]);
     }
}
/* }}} */
/* {{{ void echo(clone_t *clone, char *chan, char *buf) */
void echo(clone_t *clone, char *chan, char *buf)
{
     register int i;
     char **pchan;

     if (echo_mode == 0)
     {
          /* Echo the to the current channel */
          send_sock(clone->sock, "PRIVMSG %s %s", chan, buf);
     }
     else
     {
          /* Echo to all the active channels */
          for (i = 0, pchan = channel; i < MAX_CHANS; ++i, ++pchan)
          {
               if (*pchan)
               {
                    send_sock(clone->sock, "PRIVMSG %s %s", *pchan, buf);
               }
          }
     }
}
/* }}} */
/* {{{ void send_irc_nick(clone_t *clone, char *nick) */
/**
 * send_irc_nick()
 * Change `clone's nick to `nick'.
 */
void send_irc_nick(clone_t *clone, char *nick)
{
     send_sock(clone->sock, "NICK %s\n", nick);
}
/* }}} */
/* }}} */
/* {{{ Functions for initializing clones */
/* {{{ void register_clone(clone_t *clone) */
void register_clone(clone_t *clone)
{
     char *ident;

     if (clone->server_ident)
     {
          ident = clone->server_ident;
     }
     else
     {
          ident = clone->ident;
     }

     send_sock(clone->sock, "USER %s 0 0 :%s\n", ident, clone->real);

     if (clone->server_pass)
     {
          send_sock(clone->sock, "PASS %s\n", clone->server_pass);
     }

     send_sock(clone->sock, "MODE %s +i\n", clone->nick);
}
/* }}} */
/* {{{ int main_clone(void *arg) */
int main_clone(void *arg)
{
     clone_t *clone;
     clone = (clone_t *) arg;

     if (conf.debug)
     {
          if (clone->type != NOSPOOF)
          {
               print(1, 0, 0, "%s launched: host=%s server=%s", strtype[clone->type], clone->proxy, clone->server);
          }
          else
          {
               print(1, 0, 0, "direct connection launched: server=%s", clone->server);
          }
     }

     if (clone->type == NOSPOOF || clone->type == VHOST)
     {
          if (connect_clone(clone, clone->server, clone->server_port))
          {
               return 1;
          }
     }
     else
     {
          if (connect_clone(clone, clone->proxy, clone->proxy_port))
          {
               return 1;
          }
     }

     clone->status = WAIT_CONNECT;
     return 0;
}
/* }}} */
/* }}} */
/* {{{ Functions for parsing and acting on that parsing */
/* {{{ int parse_deco(clone_t *clone, char *buf) */
int parse_deco(clone_t *clone, char *buf)
{
     if (!clone->online)
     {
          if (!StrCmpPrefix(buf, "ERROR"))
          {
               if (strstr(buf, "Too many user connections") || strstr(buf, "Too many host connections") ||
                   strstr(buf, "Too many connections") || strstr(buf, "This server is full") || strstr(buf, "Invalid username")) /* 468 = invalidusername */
               {
                    if (clone->save)
                    {
                         save_host(clone);
                    }
               }
          }
          if (conf.debug)
          {
               print_prefix(clone, 4, 0);
               print(0, 4, 0, "%s", buf);
          }
          return 1;
     }

     clone->online = 0;

     if (conf.verbose)
     {
          print_prefix(clone, 4, 0);
          print(0, 4, 0, "%s", buf);
     }

     if (conf.max_reco < 0 || clone->reco < conf.max_reco)
     {
          if (conf.verbose)
          {
               print_prefix(clone, 0, 0);
               print(1, 0, 0, "is trying to reconnect");
          }
          clone->alarm = time(NULL) + conf.wait_reco;
          if (conf.max_reco > 0)
          {
               ++clone->reco;
          }
          close_sock(clone);
          main_clone(clone);
          return 0;
     }

     return 1;
}
/* }}} */
/* {{{ void save_host(clone_t *clone) */
void save_host(clone_t *clone)
{
     FILE *f;
     char ip[MEDBUF];
     char line[MEDBUF];

     if (clone->type == NOSPOOF)
     {
          return;
     }

     host2ip(ip, clone->proxy, sizeof(ip));

     if (clone->type == PROXY || clone->type == SOCKS4 || clone->type == SOCKS5)
     {
          snprintf(line, sizeof(line), "%s:%d\n", ip, clone->proxy_port);
     }
     else
     {
          snprintf(line, sizeof(line), "%s\n", ip);
     }

     if (!occur_file(line, clone->save) && (f = fopen(clone->save, "a+")))
     {
          fputs(line, f);
          fclose(f);
     }
}
/* }}} */
/* {{{ void parse_irc_connect(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_connect(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     int chid;

     if (clone->save)
     {
          save_host(clone);
     }

     if (clone->type != NOSPOOF)
     {
          print(1, 3, 0, "clone available: nick=%s; %s=%s; server=%s", clone->nick, strtype[clone->type], clone->proxy, clone->server);
     }
     else
     {
          print(1, 3, 0, "clone available: nick=%s; server=%s", clone->nick, clone->server);
     }

     if (conf.scan || clone->mode == M_QUIT)
     {
          free_clone(clone);
          return;
     }

     clone->online = 1;

     if (clone->mode == M_NORMAL)
     {
          for (chid = 0; chid < MAX_CHANS; ++chid)
          {
               if (channel[chid])
               {
                    join(clone, channel[chid]);
               }
          }
     }
}
/* }}} */
/* {{{ Jupe functions */
/* {{{ void parse_irc_isupport(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_isupport(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     char *mspec_str = NULL;

     if (NULL != (mspec_str = strstr(buf, "MONITOR=")))
     {
          sscanf(mspec_str, "MONITOR=%u", &(clone->monitor_tmax));
          if (0 == clone->monitor_tmax)
          {
               return;
          }

          clone->jupes = xmalloc((conf.nick_length + 1) * clone->monitor_tmax);
          bzero(clone->jupes, (conf.nick_length + 1) * clone->monitor_tmax);
     }
}
/* }}} */
/* {{{ void parse_irc_moffline(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_moffline(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     char *pnick0 = buf;
     char *pnick = pnick0;

     if(!((NULL != (pnick0 = strchr(pnick0, ':'))) && *(++pnick0)))
     {
          return;
     }

     pnick = pnick0;
     strsep(&(pnick0), "\r\n");
     pnick0 = pnick;

     while(NULL != (pnick = strsep(&(pnick0), ",")))
     {
          do_jupe(clone, pnick);
     }
}
/* }}} */
/* {{{ void do_jupe(clone_t *clone, char *nick) */
void do_jupe(clone_t *clone, char *nick)
{
     int i;
     clone_t **pcl;

     if (occur_table(nick, conf.juping, MAX_JUPES))
     {
          return;
     }

     print_prefix(clone, 0, 0);
     print(1, 0, 0, "nickname `%s' is available, trying to grab", nick);

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if ((*pcl) && (*pcl)->online && (*pcl)->grabbing == 0)
          {
               send_irc_nick((*pcl), nick);
               print_prefix((*pcl), 0, 0);
               print(1, 0, 0, "Nice nick, I think I'll take it.");
               (*pcl)->grabbing = 1;
               add_table(nick, conf.juping, MAX_JUPES);
               return;
          }
     }

     print_prefix(clone, 0, 0);
     print(1, 0, 0, "no clones left that aren't juping", nick);
}
/* }}} */
/* }}} */
/* {{{ void parse_irc_kickban(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_kickban(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     char *toban;
     char *tokick;

     if (!clone->wait_whois)
     {
          return;
     }

     clone->lastsend = time(NULL);
     clone->wait_whois = 0;

     strsep(&buf, DELIM);
     if (!(tokick = strsep(&buf, DELIM)))
     {
          return;
     }

     strsep(&buf, DELIM);
     if (!(toban = strsep(&buf, DELIM)))
     {
          return;
     }

     ban(clone, mass_ch, toban, 0);
     kick(clone, mass_ch, tokick, mass_reas ? mass_reas : NULL, 0);
}
/* }}} */
/* {{{ void parse_irc_nick(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_nick(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     char *dest;
     int chid;

     strsep(&buf, ":");
     if (!(dest = strsep(&buf, DELIM)))
     {
          return;
     }

     if (!StrCompare(nick, clone->nick))
     {
          StrCopy(clone->nick, dest, conf.nick_length+1);
     }

     if (target && !StrCompare(nick, target))
     {
          free(target);
          target = StrDuplicate(dest);
     }

     update_table(nick, dest, broth, MAX_BROTHERS);
     update_table(nick, dest, conf.prot, MAX_PROTS);

     update_pattern_table(nick, dest, conf.aop, MAX_AOPS);

     for (chid = 0; chid < MAX_CHANS; ++chid)
     {
          update_queue(nick, dest, &names[chid]);
          update_queue(nick, dest, &names_op[chid]);
     }
}
/* }}} */
/* {{{ void parse_irc_control(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_control(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     char buffer[BIGBUF];
     char is_aop;

     snprintf(buffer, sizeof(buffer), "%s %s %s %s", from, cmd, clone->nick, buf);
     is_aop = (match_table(from, conf.aop, MAX_AOPS) != -1);

     if (is_aop && conf.notice)
     {
          if (conf.verbose)
          {
               print(0, 5, 0, "%s", buffer);
          }
          buf = strstr(buffer, " :") + 2; /* don't interfere with IPv6 IP */
          op_nick = StrDuplicate(nick);
          interpret(buf, -clone->id - 2);
     }
}
/* }}} */
/* {{{ void parse_irc_notice(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_notice(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     char parm[MINIBUF];
     strsep(&buf, DELIM);

     /* Skip mIRC DCC info */
     if (!StrParam(parm, sizeof(parm), buf, 0))
     {
          if (!StrCompare(parm, ":dcc"))
          {
               return;
          }
     }

     parse_irc_control (clone, cmd, nick, from, buf);
}
/* }}} */
/* {{{ void parse_irc_privmsg(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_privmsg(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     int chid;
     char *dest;
     char *parm;

     dest = strsep(&buf, DELIM);
     if (!StrCompare(dest, clone->nick))
     {
          parse_irc_control(clone, cmd, nick, from, buf);
          return;
     }

     if (target && !StrCompare(nick,target))
     {
          echo(clone, dest, buf);
          return;
     }

     if ((chid = getchid(dest)) == -1)
     {
          return;
     }

     if (conf.aggressive && is_enemy(nick))
     {
          kick(clone, chid, nick, NULL, 1);
          return;
     }

     if (!(parm = strsep(&buf, DELIM)))
     {
          return;
     }

     if (!StrCompare(parm,":!op"))
     {
          if (match_table(from, conf.aop, MAX_AOPS) != -1)
          {
               if (clone->op[chid])
               {
                    op(clone, chid, nick);
               }
          }
     }
}
/* }}} */
/* {{{ void join_scan(scan_t *scan, char *from) */
void join_scan(scan_t *scan, char *from)
{
     char *host;
     char ip[MINIBUF];
     char line[MEDBUF];

     if (!(host = strchr(from, '@')))
     {
          return;
     }

     host2ip(ip, ++host, sizeof(ip));

     if (scan->type == PROXY)
     {
          snprintf(line, sizeof(line), "%s:%d\n", ip, scan->proxy_port);
     }
     else
     {
          snprintf(line, sizeof(line), "%s\n", ip);
     }

     if (!occur_file(line, scan->save))
     {
          load_host(scan->type, host, scan->proxy_port, scan->server, scan->server_port, NULL, NULL, scan->save, scan->mode);
     }
}
/* }}} */
/* {{{ void parse_irc_join(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_join(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     int i;
     int is_aop;
     int chid;
     char *chan;
     char *reas;

     strsep(&buf, ":");
     chan = strsep(&buf, DELIM);

     if ((chid = getchid(chan)) == -1)
     {
          return;
     }

     if (!StrCompare(nick, clone->nick))
     {
          if (!clone->restricted)
          {
               clone->needop[chid] = 1;
          }
          return;
     }

     is_aop = (match_table(from, conf.aop, MAX_AOPS) != -1);
     if (is_aop && !occur_table(nick, conf.prot, MAX_PROTS))
     {
          if (add_table(nick, conf.prot, MAX_PROTS) != -1)
          {
               if (conf.verbose)
               {
                    print(1, 0, 0, "%s added to the protected nicks (aop)", nick);
               }
          }
     }

     uniq_add_queue(nick, &names[chid]);

     if (clone->op[chid])
     {
          if (is_aop || !not_a_clone(nick))
          {
               op(clone, chid, nick);
               return;
          }
          if ((i = match_table(from, conf.shit, MAX_SHITS)) != -1)
          {
               ban(clone, chid, from, 1);
               if ((reas = strchr(conf.shit[i], ':')))
               {
                    kick(clone, chid, nick, reas+1, 1);
               }
               else
               {
                    kick(clone, chid, nick, SHIT_REAS, 1);
               }
               return;
          }
     }

     if (clone->scan)
     {
          join_scan(clone->scan, from);
     }
}
/* }}} */
/* {{{ void parse_irc_error_invite(clone_t *clone, int chid) */
void parse_irc_error_invite(clone_t *clone, int chid)
{
     clone_t *one;

     if ((one = getop(chid)))
     {
          send_sock(one->sock, "INVITE %s %s\n", clone->nick, channel[chid]);
     }
}
/* }}} */
/* {{{ void parse_irc_error(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_error(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     int chid;
     char *chan;
     char buffer[BIGBUF];

     snprintf(buffer, sizeof(buffer), "%s %s %s", from, cmd, buf);

     if (conf.verbose)
     {
          print(0, 5, 0, "%s", buffer);
     }

     if (!StrCompare(cmd,"484")) /* ischanservice */
     {
          clone->restricted = 1;
          return;
     }

     strsep(&buf, DELIM);
     chan = strsep(&buf, DELIM);

     if ((chid = getchid(chan)) == -1)
     {
          return;
     }

     if (!StrCompare(cmd,"471") || !StrCompare(cmd,"473")) /* +l or +i */
     {
          parse_irc_error_invite(clone, chid);
     }

     if (!StrCompare(cmd,"372") || !StrCompare(cmd,"375") || !StrCompare(cmd,"376")) /* MOTD numerics. */
     {
          return; /* These errors would otherwise kill good clones. */
     }

     clone->alarm = time(NULL) + conf.rejoin;

     if (occur_table(channel[chid], channel, MAX_CHANS))
     {
          join(clone, channel[chid]);
     }
}
/* }}} */
/* {{{ void parse_irc_kick(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_kick(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     int chid;
     char *chan;
     char *kicked;

     chan = strsep(&buf, DELIM);
     kicked = strsep(&buf, DELIM);

     if ((chid = getchid(chan)) == -1)
     {
          return;
     }

     remove_queue(kicked, &names_op[chid]);
     remove_queue(kicked, &names[chid]);

     if (!StrCompare(kicked, clone->nick))
     {
          clone->op[chid] = 0;
          clone->needop[chid] = 0;
          join(clone, channel[chid]);
     }
     else if (!conf.peace && clone->op[chid])
     {
          if (!conf.aggressive)
          {
               deop_enemy(clone, chid, nick);
          }
          else
          {
               massdeop(clone, chid);
          }
     }
}
/* }}} */
/* {{{ void parse_irc_part(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_part(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     char *chan;
     int chid;

     chan = strsep(&buf, DELIM);
     if ((chid = getchid(chan)) == -1)
     {
          return;
     }

     remove_queue(nick, &names_op[chid]);
     remove_queue(nick, &names[chid]);
}
/* }}} */
/* {{{ void parse_irc_mode(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_mode(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     int chid;
     char *chan;
     char *mode;
     char *parm;
     int i;
     int x = 1;        /* by default it's + */

     if (clone->restricted)
     {
          return;
     }

     chan = strsep(&buf, DELIM);
     mode = strsep(&buf, DELIM);

     if (!StrCompare(chan, clone->nick))
     {
          return;
     }

     if ((chid = getchid(chan)) == -1)
     {
          return;
     }

     if (conf.aggressive && !clone->op[chid])
     {
          clone->needop[chid] = 1;
     }

     for (i = 0; mode[i]; ++i)
     {
          switch (mode[i])
          {
               case '+':
                    x = 1;
                    break;
               case '-':
                    x = -1;
                    break;
               case 'b':
               case 'e':
               case 'I':
               case 'k':
               case 'l':
               case 'h':        /* do we really need this? */
               case 'v':
                    parm = strsep(&buf, DELIM);
                    break;
               case 'o':
                    parm = strsep(&buf, DELIM);
                    if (x == 1)        /* +o */
                    {
                         uniq_add_queue(parm, &names_op[chid]);
                         remove_queue(parm, &names[chid]);
                         if (!clone->op[chid] && !StrCompare(clone->nick, parm))
                         {
                              clone->op[chid] = 1;
                              clone->needop[chid] = 0;
                              massop(clone, chid);
                         }
                         if (!conf.aggressive && !conf.peace)
                         {
                              if (clone->op[chid] && deop_enemy(clone, chid, nick))
                              {
                                   if (is_enemy(parm))
                                   {
                                        deop(clone, chid, parm);
                                   }
                              }
                         }
                    }
                    else                /* -o */
                    {
                         uniq_add_queue(parm, &names[chid]);
                         remove_queue(parm, &names_op[chid]);
                         if (clone->op[chid] && !StrCompare(clone->nick, parm))
                         {
                              clone->op[chid] = 0;
                              clone->needop[chid] = 1;
                         }
                         else if (clone->op[chid])
                         {
                              if (!conf.aggressive && !conf.peace)
                              {
                                   deop_enemy(clone, chid, nick);
                              }
                              if (StrCompare(nick, parm) && !is_enemy(parm))
                              {
                                   op(clone, chid, parm);
                              }
                         }
                    }
               break;
          }
     }

     if (conf.aggressive && !conf.peace && clone->op[chid])
     {
          massdeop(clone, chid);
     }
}
/* }}} */
/* {{{ void parse_irc_names(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_names(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     int chid;
     char *chan;
     char parm[MINIBUF];
     register int i;

     strsep(&buf, DELIM);
     strsep(&buf, DELIM);
     chan = strsep(&buf, DELIM);
     strsep(&buf, ":");

     if ((chid = getchid(chan)) == -1)
     {
          return;
     }

     for (i = 0; !StrParam(parm, sizeof(parm), buf, i); ++i)
     {
          if (parm[0] == '@')
          {
               uniq_add_queue(parm+1, &names_op[chid]);
          }
          else if (parm[0] == '+')
          {
               uniq_add_queue(parm+1, &names[chid]);
          }
          else
          {
               uniq_add_queue(parm, &names[chid]);
          }
     }

     if (clone->restricted)
     {
          return;
     }

     snprintf(parm, sizeof(parm), "@%s", clone->nick);
     if (is_in(parm, buf))
     {
          clone->op[chid] = 1;
          clone->needop[chid] = 0;
     }
}
/* }}} */
/* {{{ void parse_irc_topic(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_topic(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     int chid;
     char *chan;

     if (conf.peace)
     {
          return;
     }

     chan = strsep(&buf, DELIM);
     if ((chid = getchid(chan)) == -1)
     {
          return;
     }

     if (clone->op[chid])
     {
          deop_enemy(clone, chid, nick);
     }
}
/* }}} */
/* {{{ void parse_irc_unban(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_unban(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     char *chan;
     char *toban;
     int chid;

     strsep(&buf, DELIM);
     chan = strsep(&buf, DELIM);

     if ((chid = getchid(chan)) == -1)
     {
          return;
     }

     if (!(toban = strsep(&buf, DELIM)))
     {
          return;
     }

     unban(clone, chid, toban);
}
/* }}} */
/* {{{ void parse_irc_quit(clone_t *clone, char *cmd, char *nick, char *from, char *buf) */
void parse_irc_quit(clone_t *clone, char *cmd, char *nick, char *from, char *buf)
{
     int chid;

     if (!remove_table(nick, conf.prot, MAX_PROTS))
     {
          if (conf.verbose)
          {
               print(1, 0, 0, "%s not protected anymore (quit)", nick);
          }
     }

     for (chid = 0; chid < MAX_CHANS; ++chid)
     {
          remove_queue(nick, &names_op[chid]);
          remove_queue(nick, &names[chid]);
     }
}
/* }}} */
/* {{{ Constants */
#define IDENT1 "IDENTIFY"
#define IDENT2 "choose another one"
#define IDENT3 "Please choose a different nickname"
/* }}} */
/* {{{ int nick_error(char *cmd, char *nick, char *buf) */
int nick_error(char *cmd, char *nick, char *buf)
{
     if (conf.dalnet && !StrCompare(nick, "nickserv"))
     {
          if (strstr(buf, IDENT1) || strstr(buf, IDENT2) || strstr(buf, IDENT3))
          {
               return 1;
          }
     }
/* XXX: There *has* to be a better way to compact this. -- Leon */
     if (!StrCompare(cmd, "431") || !StrCompare(cmd, "432") || !StrCompare(cmd, "433") || !StrCompare(cmd, "436") || !StrCompare(cmd, "437") || !StrCompare(cmd, "438"))
     {
          return 1;
     }

     return 0;
}
/* }}} */
/* {{{ void parse_irc(clone_t *clone, char *buf) */
void parse_irc(clone_t *clone, char *buf)
{
     char *from;
     char *parm;
     char nick[MINIBUF+1];
/* NOTE: 431, 432, 433, 436, 437, & 438 are covered by ``nick_error()'' */
     static msg_t msg[] =
          { { "JOIN",     parse_irc_join         },
            { "PART",     parse_irc_part         },
            { "QUIT",     parse_irc_quit         },
            { "NICK",     parse_irc_nick         },
            { "PRIVMSG",  parse_irc_privmsg      },
            { "NOTICE",   parse_irc_notice       },
            { "KICK",     parse_irc_kick         },
            { "MODE",     parse_irc_mode         },
            { "TOPIC",    parse_irc_topic        },
            { "001",      parse_irc_connect      },
            { "005",      parse_irc_isupport     },
            { "311",      parse_irc_kickban      },
            { "353",      parse_irc_names        },
            { "367",      parse_irc_unban        },
            { "372",      parse_irc_error        },
            { "471",      parse_irc_error        },
            { "472",      parse_irc_error        },
            { "473",      parse_irc_error        },
            { "474",      parse_irc_error        },
            { "475",      parse_irc_error        },
            { "484",      parse_irc_error        },
            { "731",      parse_irc_moffline     },
            { NULL,       NULL,                  }
          };
     msg_t *pmsg;
/* TODO:
   367 => "banlist",
   368 => "endofbanlist",
   404 => "cannotsendtochan",
   "INVITE"/"KILL"
*/

     if (!StrCmpPrefix(buf, "PING"))
     {
          buf[1] = 'O';
          send(clone->sock, buf, strlen(buf), 0);
          return;
     }

     if (!strsep(&buf, ":"))
     {
          return;
     }

     if (!(from = strsep(&buf, DELIM)))
     {
          return;
     }

     if (!(parm = strsep(&buf, DELIM)))
     {
          return;
     }

     if (strchr(from, '!'))
     {
          sscanf(from, "%"MINIBUF_TXT"[^!]", nick);
     }
     else
     {
          nick[0] = 0;
     }

     if (nick_error(parm, nick, buf))
     {
          randget(clone, &clone->nick2, conf.nick_length, conf.use_wordlist, conf.nicks, MAX_NICKS);
          if (conf.verbose)
          {
               print(1, 0, 0, "nickname error for %s, changing to %s.", clone->nick, clone->nick2);
          }
          if (!clone->online)
          {
               StrCopy(clone->nick, clone->nick2, conf.nick_length+1);
          }
          send_irc_nick(clone, clone->nick2);
          free(clone->nick2);
          return;
     }

     for (pmsg = msg; pmsg->parm; ++pmsg)
     {
          if (!StrCompare(parm, pmsg->parm))
          {
               pmsg->function(clone, parm, nick, from, buf);
               return;
          }
     }
}
/* }}} */
/* }}} */
/* {{{ send_*() functions */
/* {{{ void send_join(char *buffer) */
void send_join(char *buffer)
{
     register int id;
     clone_t **pcl;
     char *chan;

     if (!strsep(&buffer, DELIM))
     {
          return;
     }

     if (!(chan=strsep(&buffer, DELIM)))
     {
          return;
     }

     for (id = 0, pcl = cl; id < MAX_CLONES; ++id, ++pcl)
     {
          if (*pcl && (*pcl)->online)
          {
               join(*pcl, chan);
          }
     }
}
/* }}} */
/* {{{ void send_nick() */
void send_nick()
{
     register int id;
     clone_t **pcl;

     for (id = 0, pcl = cl; id < MAX_CLONES; ++id, ++pcl)
     {
          if (*pcl && (*pcl)->online && (*pcl)->grabbing == 0)
          {
               randget(*pcl, &(*pcl)->nick2, conf.nick_length, conf.use_wordlist, conf.nicks, MAX_NICKS);
               send_irc_nick(*pcl, (*pcl)->nick2);
               free((*pcl)->nick2);
          }
     }
}
/* }}} */
/* {{{ void send2clones(char *buffer) */
void send2clones(char *buffer)
{
     register int id;
     clone_t **pcl;

     if (!StrCmpPrefix(buffer, "join"))
     {
          send_join(buffer);
     }
     else if (!StrCmpPrefix(buffer, NICKS))
     {
          send_nick();
     }
     else
     {
          for (id = 0, pcl = cl; id < MAX_CLONES; ++id, ++pcl)
          {
               if (*pcl && (*pcl)->online)
               {
                    send((*pcl)->sock, buffer, strlen(buffer), 0);
               }
          }
     }
}
/* }}} */
/* {{{ void send2server(clone_t *clone, const char *fmt, ...) */
void send2server(clone_t *clone, const char *fmt, ...)
{
     static int lastlock = 0;
     clone_t **pcl;
     time_t now;
     int i;
     int j;
     int n;
     char tosend[BIGBUF];
     va_list ap;

     now = time(NULL);

     /* Synchronization of the queues */
     if (lastlock != now)
     {
          lastlock = now;
          clear_queue(&public_queue);

          for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
          {
               if (*pcl)
               {
                    clear_queue(&(*pcl)->queue);
               }
          }
     }

     va_start(ap, fmt);
     n = vsnprintf(tosend, sizeof(tosend), fmt, ap);
     va_end(ap);

     /* Each clone has its own send queue. */
     /* Before trying to send anything, it puts it in. */
     add_queue(tosend, &clone->queue);

     /* If the public queue has more occurences of the message, it has */
     /* already been sent to IRC. */
     i = occur_queue(tosend, &clone->queue) + conf.repeat;
     j = occur_queue(tosend, &public_queue);

     if (i <= j)
     {
          return;
     }

     add_queue(tosend, &public_queue);

     send(clone->sock, tosend, strlen(tosend), 0);
     clone->lastsend = now;
}
/* }}} */
/* }}} */
