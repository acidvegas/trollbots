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

#ifndef CLONE_H
#define CLONE_H

#include "init.h"
#include "net.h"
#include "lists.h"

typedef struct
{
     int type;
     char mode;
     char *save;
     char *server;
     unsigned short proxy_port;
     unsigned short server_port;
} scan_t;

typedef struct
{
     int id;
     int type;
     char *proxy;
     unsigned short proxy_port;
     char *server;
     unsigned short server_port;
     char *server_pass;
     char *server_ident;
     char *host;
     unsigned short port;
     int sock;
     char *save;
     char online;
     char restricted;
     int reco;
     unsigned int monitor_tmax;
     int grabbing;
     char **jupes;
     char op[MAX_CHANS];
     char needop[MAX_CHANS];
     char *nick;
     char *nick2;
     char *ident;
     char *real;
     queue queue;
     char wait_whois;
     scan_t *scan;
     char mode;
     char status;
     time_t lastsend;
     time_t start;
     time_t alarm;
     time_t rejoin_time;
     char buffer[1024];
     char lastbuffer[1024];
} clone_t;

typedef struct
{
     char *parm;
     void (*function) (clone_t *cl, char *cmd, char *nick, char *from, char *buf);
} msg_t;

void free_clone (clone_t *clone);
int main_clone (void *arg);
void save_host (clone_t *clone);
int not_a_clone (char *s);
int not_a_mast (char *s);
int is_enemy (char *s);
int is_op (clone_t *clone, int chid);
void op (clone_t *clone, int chid, char *s);
void deop (clone_t *clone, int chid, char *s);
int deop_enemy (clone_t *clone, int chid, char *nick);
void kick (clone_t *clone, int chid, char *s, char *reas, int mode);
void ban (clone_t *clone, int chid, char *s, int mode);
void unban (clone_t *clone, int chid, char *s);
void kickban (clone_t *clone, char *s);
void join (clone_t *clone, char *dest);
void echo (clone_t *clone, char *chan, char *buf);
void send_irc_nick (clone_t *clone, char *nick);
void register_clone (clone_t *clone);

void send2clones (char *buffer);
void send2server (clone_t *clone, const char *fmt, ...);

int parse_deco (clone_t *clone, char *buf);
void parse_irc (clone_t *clone, char *buf);

#endif
