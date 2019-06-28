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

#ifndef PROXY_H
#define PROXY_H

#include "clone.h"

enum states {
     INIT,
     WAIT_CONNECT,
     WAIT_IDENT,
     WAIT_IRC,
     WAIT_SOCKS4,
     WAIT_SOCKS5_1,
     WAIT_SOCKS5_2,
     WAIT_PROXY,
     WAIT_CISCO,
     WAIT_PASS,
     WAIT_HOST,
     WAIT_CMD,
     EXIT
};

int connect_clone (clone_t *clone, char *host, unsigned short port);
int init_vhost (int sock, char *vhost);
int init_socks4 (clone_t *clone);
int init_socks5 (clone_t *clone);
int init_read_socks5 (clone_t *clone);
int init_proxy (clone_t *clone);
int init_bouncer (clone_t *clone);
int read_cisco (clone_t *clone);
int readline (int s, char *buffer, size_t buffer_size);
int read_proxy (clone_t *clone);
int read_socks4 (clone_t *clone);
int read_socks5 (clone_t *clone);
void sendnick (clone_t *clone, char *nick);
void senduser (clone_t *clone);

#endif
