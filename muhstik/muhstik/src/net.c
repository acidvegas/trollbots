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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "../include/net.h"
#include "../include/proxy.h"
#include "../include/string.h"
#include "../include/print.h"
/* }}} */
/* {{{ Variables */
/* {{{ Global Variables */
int maxsock;
/* }}} */
/* }}} */
/* {{{ Hard-wired, compile-time constants */
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif

#define MAX_HOSTNAME 256
/* }}} */
/* {{{ lookup_*() */
/* ``he->h_addrtype == AF_INET6`` */
int lookup_host(char *host, struct in_addr *addr)
{
     struct hostent *he;

     if ((he = gethostbyname(host)) == NULL)
     {
          return 1; /* In ``telnet.c'' it was ``return 0;''... -- Leon */
     }

     memcpy(addr, he->h_addr, he->h_length);
     return 0; /* In ``telnet.c'' it was ``return 1;''... -- Leon */
}

int lookup_ip(char *host, struct in_addr *addr)
{ /* XXX: Does this support IPv6 IPs? -- Leon */
     /* Use inet_addr for portability to Solaris */
     /* Fuck Solaris -- Leon */
     return (((*addr).s_addr = inet_addr(host)) == INADDR_NONE);
}
/* }}} */
/* {{{ DNS-related functions */
int resolve(char *host, struct in_addr *addr)
{
     return (lookup_host(host,addr) && lookup_ip(host,addr));
}

void host2ip(char *s, char *host, int size)
{ /* TODO: Support IPv6 IPs, I guess. -- Leon */
     struct in_addr addr;
     u_char *p;

     if (resolve(host, &addr))
     {
          StrCopy(s, host, size);
          return;
     }
     p = (u_char *) &addr.s_addr;
     snprintf(s, size, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
}
/* }}} */
/* {{{ net_*() */
/* {{{ net_set-type shit */
void net_set_socket_options(int sock)
{
     int sw;
     sw = 1;
     setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &sw, sizeof(sw));
     sw = 1;
     setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *) &sw, sizeof(sw));
     sw = 0;
     setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *) &sw, sizeof(sw));
}

void set_nonblocking(int sock)
{
     int opts;

     opts = fcntl(sock, F_GETFL);
     if (opts < 0)
     {
          print_error("fcntl(F_GETFL)");
          exit(EXIT_FAILURE);
     }

     opts = (opts | O_NONBLOCK);
     if (fcntl(sock, F_SETFL, opts) < 0)
     {
          print_error("fcntl(F_SETFL)");
          exit(EXIT_FAILURE);
     }
}
/* }}} */
/* {{{ net_store*() */
netstore *net_store_new()
{
     netstore *ns;
     ns = xmalloc(sizeof(netstore));
     memset(ns, 0, sizeof(netstore));
     return ns;
}

void net_store_destroy(netstore *ns)
{
     if (ns->ip6_hostent)
     {
          freeaddrinfo(ns->ip6_hostent);
     }
     free(ns);
}
/* }}} */
/* {{{ net_socket() */
int net_socket(int *sock, int type)
{
     if ((*sock = socket(type, SOCK_STREAM, IPPROTO_TCP)) == -1)
     {
          return 1;
     }
     maxsock = max(*sock, maxsock);
     net_set_socket_options(*sock);
     set_nonblocking(*sock);
     return 0;
}
/* }}} */
/* {{{ Resloving, connecting, binding functions */
int net_resolve(netstore *ns, char *hostname, unsigned short port)
{
     struct addrinfo hints;
     char portstring[MAX_HOSTNAME];

     snprintf(portstring, MAX_HOSTNAME, "%d", port);
     memset(&hints, 0, sizeof (struct addrinfo));
     hints.ai_family = PF_UNSPEC; /* support IPv6 and IPv4 */
     hints.ai_flags = AI_CANONNAME;
     hints.ai_socktype = SOCK_STREAM;

     return getaddrinfo(hostname, port ? portstring : NULL, &hints, &ns->ip6_hostent);
}

int net_connect(netstore *ns, int *sock, char *vhost)
{
     struct addrinfo *res;
     struct addrinfo *res0;

     res0 = ns->ip6_hostent;
     for (res = res0; res; res = res->ai_next)
     {
          switch (res->ai_family)
          {
               case AF_INET:
               case AF_INET6:
                    if (net_socket(sock, res->ai_family))
                    {
                         return -1;
                    }

                    if (vhost && init_vhost(*sock, vhost))
                    {
                         return -1;
                    }

                    return connect(*sock, res->ai_addr, res->ai_addrlen);
          }
     }
     return -1;
}

int net_bind(netstore *ns, int sock)
{
     return bind(sock, ns->ip6_hostent->ai_addr, ns->ip6_hostent->ai_addrlen);
}
/* }}} */
/* }}} */
/* {{{ send_sock() */
void send_sock(int sock, const char *fmt, ...)
{
     char buffer[BIGBUF];
     int n;
     va_list ap;

     va_start(ap, fmt);
     n = vsnprintf(buffer, sizeof(buffer), fmt, ap);
     va_end(ap);

     send(sock, buffer, strlen(buffer), 0);
}
/* }}} */
