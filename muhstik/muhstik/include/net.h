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

#ifndef NETWORK_H
#define NETWORK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define max(n1, n2)\
(n1 > n2 ? n1 : n2)

typedef struct
{
     struct addrinfo *ip6_hostent;
} netstore;

void host2ip (char *s, char *host, int size);
int resolve (char *host, struct in_addr *addr);
netstore *net_store_new ();
void net_store_destroy (netstore *ns);
void net_set_socket_options (int sock);
void set_nonblocking (int sock);
int net_bind (netstore *ns, int sock);
int net_resolve (netstore *ns, char *hostname, unsigned short port);
int net_connect (netstore *ns, int *sock, char *vhost);
void send_sock (int sock, const char *fmt, ...);

#endif
