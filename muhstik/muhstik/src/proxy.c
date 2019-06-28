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

/* {{{ Relevant Information:
 * NOTE: Much of the following miles upon miles of utterly worthless blogging
 *        was taken from the BOPM source code.
 *
 *  Cisco Routers: Cisco routers with a default password. Also pretty much
 *                   anything else that will let you `telnet' to anywhere else
 *                   on the Internet. These are (apparently) always on port 23.
 *
 *  HTTP/1.1 {POST,CONNECT}:
 *
 *  HTTP POST: The HTTP POST protocol used to often be dismissed when writing
 *              the access controls for proxies until a massive series of GNAA
 *              crapfloods.
 *             HTTP POST Offers only the opportunity to send a single block of
 *              data, but enough of them at once can still make for a
 *              devastating flood (see: `GNAA'.) Found on the same ports that
 *              HTTP CONNECT proxies inhabit.
 *             Note that if an ircd has "ping cookies" then clients from HTTP
 *              POST proxies cannot connect to the network anyway.
 *
 *  HTTP CONNECT: A very common proxy protocol supported by widely known
 *                 software such as Squid and Apache. The most common sort of
 *                 insecure proxy and found on a multitude of weird ports too.
 *                Offers transparent two way TCP connections.
 *
 *             [RFC2616] Hypertext Transfer Protocol -- HTTP/1.1                     @  <http://tools.ietf.org/txt/rfc2616.txt>
 *             [RFC2617] HTTP Authentication: Basic and Digest Access Authentication @  <http://tools.ietf.org/txt/rfc2617.txt>
 *
 *  SOCKS{4,5}: Well known proxy protocols, probably the second most common for
 *               insecure proxies, also offers transparent two way TCP
 *               connections. Largely confined to port 1080.
 *
 *  SOCKS4:
 *             idk                                                                   @  <http://socks.permeo.com/protocol/socks4.protocol>
 *             SOCKS: A protocol for TCP proxy across firewalls                      @  <http://www.digitalcybersoft.com/ProxyList/socks4.shtml>
 *             SOCKS 4A: A Simple Extension to SOCKS 4 Protocol                      @  <http://www.digitalcybersoft.com/ProxyList/socks4a.shtml>
 *
 *  SOCKS5:
 *             [RFC1928] SOCKS Protocol Version 5                                    @  <http://tools.ietf.org/txt/rfc1928.txt>
 *             [RFC1929] Username/Password Authentication for SOCKS V5               @  <http://tools.ietf.org/txt/rfc1929.txt>
 *             [RFC1961] GSS-API Authentication Method for SOCKS Version 5           @  <http://tools.ietf.org/txt/rfc1961.txt>
 *             Challenge-Handshake Authentication Protocol for SOCKS V5              @  <http://www.tools.ietf.org/html/draft-ietf-aft-socks-chap>
 *
 *  TOR:
 *             idk                                                                   @  <http://www.sectoor.de/tor.php>
 *             Tor's extensions to the SOCKS protocol                                @  <https://git.torproject.org/checkout/tor/master/doc/spec/socks-extensions.txt>
 *             Design For A Tor DNS-based Exit List                                  @  <https://git.torproject.org/checkout/tor/master/doc/contrib/torel-design.txt>
 *
 *  Misc:
 *             [RFC3089] A SOCKS-based IPv6/IPv4 Gateway Mechanism                   @  <http://tools.ietf.org/txt/rfc3089.txt>
 }}} */
/* {{{ Header includes */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "../include/proxy.h"
#include "../include/print.h"
#include "../include/net.h"
#include "../include/string.h"
#include "../include/print.h"
/* }}} */
/* {{{ Variables */
/* {{{ External variables */
extern config_t conf;
extern clone_t *cl[];
/* }}} */
/* {{{ Constants */
const char *strtype[] =
{
     "SOCKS4",
     "SOCKS5",
     "proxy",
     "cisco",
     "vhost",
     "direct"
};
/* }}} */
/* }}} */
/* {{{ connect_clone() */
int connect_clone(clone_t *clone, char *host, unsigned short port)
{
     char *vhost = NULL;
     netstore *ns = net_store_new();

     if (clone->type == VHOST)
     {
          vhost = clone->proxy;
     }

     if (net_resolve(ns, host, port))
     {
          print(1, 4, 0, "%s: %s: nslookup failed", strtype[clone->type], host);
          net_store_destroy(ns);
          return 1;
     }
     if (net_connect(ns, &clone->sock, vhost))
     {
          if (errno != EINPROGRESS)
          {
               net_store_destroy(ns);
               return 1;
          }
     }
     clone->start = time(NULL);
     net_store_destroy(ns);
     return 0;
}
/* }}} */
/* {{{ init_*() */
int init_irc(clone_t *clone)
{
     send_irc_nick(clone, clone->nick);
     register_clone(clone);
     return WAIT_IRC;
}

int init_vhost(int sock, char *vhost)
{
     netstore *ns = net_store_new();

     if (net_resolve(ns, vhost, 0))
     {
          print(1, 4, 0, "vhost: %s: nslookup failed", vhost);
          net_store_destroy(ns);
          return 1;
     }
     if (net_bind(ns, sock))
     {
          print(1, 0, 0, "vhost: bind: %s", strerror(errno));
          net_store_destroy(ns);
          return 1;
     }

     net_store_destroy(ns);
     return 0;
}
/* {{{ SOCKS{4,5} */
/* {{{ CONNECT request byte order for SOCKS4
 *
 *              +----+----+----+----+----+----+----+----+----+----+....+----+
 *              | VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
 *              +----+----+----+----+----+----+----+----+----+----+....+----+
 * # of bytes:	   1    1      2              4           variable       1
 *
 *
 * Responses:
 *              +----+----+----+----+----+----+----+----+
 *              | VN | CD | DSTPORT |      DSTIP        |
 *              +----+----+----+----+----+----+----+----+
 * # of bytes:	  1    1      2              4
 *
 * VN is the version of the reply code and should be 0. CD is the result
 * code with one of the following values:
 *
 * 90: Request granted.
 * 91: Request rejected or failed.
 * 92: Request rejected because SOCKS server cannot connect to identd on the
 *      client.
 * 93: Request rejected because the client program and identd report different
 *      user-IDs.
 *
 }}} */
int init_socks4(clone_t *clone)
{
     char buffer[9];
     struct in_addr addr;
     unsigned short port;

     port = htons(clone->port);
     if (resolve(clone->host, &addr))
     {
          print(1, 4, 0, "%s: %s: nslookup failed", strtype[clone->type], clone->host);
          clone->status = EXIT;
     }
     memcpy(&buffer[2], &port, 2);
     memcpy(&buffer[4], &addr.s_addr, 4);
     buffer[0] = 4;
     buffer[1] = 1;
     buffer[8] = 0;
     send(clone->sock, buffer, 9, 0);
     return WAIT_SOCKS4;
}

/* {{{ Send version authentication selection message to SOCKS5
 *
 *       +----+----------+----------+
 *       |VER | NMETHODS | METHODS  |
 *       +----+----------+----------+
 *       | 1  |    1     | 1 to 255 |
 *       +----+----------+----------+
 *
 *  VER always contains 5, for SOCKSv5
 *  Method 0 is 'No authentication required'
 *
 *
 *
 *  The SOCKS request is formed as follows:
 *
 *        +----+-----+-------+------+----------+----------+
 *       |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
 *       +----+-----+-------+------+----------+----------+
 *       | 1  |  1  | X'00' |  1   | Variable |    2     |
 *       +----+-----+-------+------+----------+----------+
 *
 *
 *         o  VER    protocol version: X'05'
 *         o  CMD
 *            o  CONNECT X'01'
 *            o  BIND X'02'
 *            o  UDP ASSOCIATE X'03'
 *         o  RSV    RESERVED
 *         o  ATYP   address type of following address
 *            o  IP V4 address: X'01'
 *            o  DOMAINNAME: X'03'
 *            o  IP V6 address: X'04'
 *         o  DST.ADDR       desired destination address
 *         o  DST.PORT desired destination port in network octet
 *            order
 *
 }}} */
struct sock5_connect1
{
     char version;
     char nmethods;
     char method;
};

int init_socks5(clone_t *clone)
{
     struct sock5_connect1 sc1;

     sc1.version = 5;
     sc1.nmethods = 1;
     sc1.method = 0;
     send(clone->sock, (char *) &sc1, 3, 0);

     return WAIT_SOCKS5_1;
}

int init_read_socks5(clone_t *clone)
{
     unsigned short port;
     unsigned char *sc2;
     unsigned int addrlen;
     unsigned int packetlen;
     char buf[10];

     if (recv(clone->sock, buf, 2, 0) < 2)
     {
          return EXIT;
     }

     if (buf[0] != 5 && buf[1] != 0)
     {
          return EXIT;
     }

     port = htons(clone->port);
     addrlen = strlen(clone->host);
     packetlen = 4 + 1 + addrlen + 2;
     sc2 = xmalloc(packetlen);
     sc2[0] = 5;                                                  /* version */
     sc2[1] = 1;                                                  /* command */
     sc2[2] = 0;                                                  /* reserved */
     sc2[3] = 3;                                                  /* address type */
     sc2[4] = (unsigned char) addrlen;                            /* hostname length */
     memcpy(sc2 + 5, clone->host, addrlen);
     memcpy(sc2 + 5 + addrlen, &port, sizeof(unsigned short));

     send(clone->sock, sc2, packetlen, 0);
     free(sc2);

     return WAIT_SOCKS5_2;
}
/* }}} */
int init_proxy(clone_t *clone)
{
     send_sock(clone->sock, "CONNECT %s:%d HTTP/1.0\r\n\r\n", clone->host, clone->port);
     return WAIT_PROXY;
}
/* }}} */
/* {{{ readline() */
int readline(int s, char *buffer, size_t buffer_size)
{
     char c;
     unsigned int i = 0; /* ``unsigned'' is not needed, stops a warning when compiled with -Wextra */

     do {
          if (1 > read(s, &c, 1))
          {
               return 0;
          }
          if (i < (buffer_size - 1))
          {
               buffer[i++] = c;
          }
     } while (c != '\n');
     buffer[i] = 0;

     return i;
}
/* }}} */
/* {{{ Cisco-related, compile-time, constants */
#define CISCO_GREET "User Access Verification"
#define CISCO_PWD "cisco"
/* }}} */
/* {{{ read_*() */
/**
 * read_cisco(): Cisco scanning
 *
 * Attempt to connect using `CISCO_PWD' as a password, then give command for
 *  telnet(1) to the scanip/scanport
 */
int read_cisco(clone_t *clone)
{
     char buf[MEDBUF];

     memset(buf, 0, sizeof(buf));
     if (!readline(clone->sock, buf, MEDBUF))
     {
          return EXIT;
     }

     if (StrCmpPrefix(buf, CISCO_GREET))
     {
          return WAIT_CISCO;
     }

     send_sock(clone->sock, "%s\n", CISCO_PWD);
     send_sock(clone->sock, "telnet %s %d\n", clone->host, clone->port);
     return WAIT_IDENT;
}

int read_proxy(clone_t *clone)
{
     char buf[MEDBUF];

     memset(buf, 0, sizeof(buf));
     if (!readline(clone->sock, buf, MEDBUF))
     {
          return EXIT;
     }

     if (memcmp(buf, "HTTP/", 5) || memcmp(buf + 9, "200", 3))
     {
          if (conf.debug)
          {
               print(0, 2, 0, "[%s;%s] PROXY: %s", clone->nick, clone->proxy, buf);
          }
          return EXIT;
     }

     return WAIT_IDENT;
}

int read_socks4(clone_t *clone)
{
     char buffer[9];

     if (recv(clone->sock, buffer, 8, 0) < 8)
     {
          return EXIT;
     }

     if (buffer[1] != 0x5A)
     {
          if (conf.debug)
          {
               print(1, 2, 0, "[%s;%s] SOCKS4: Connection refused", clone->nick, clone->proxy);
          }
          return EXIT;
     }

     if (conf.debug)
     {
          print(1, 2, 0, "[%s;%s] SOCKS4: Success", clone->nick, clone->proxy);
     }
     return WAIT_IDENT;
}

int read_socks5(clone_t *clone)
{
     unsigned char buf[MEDBUF];
     unsigned int packetlen;

     /* consume all of the reply */
     if (recv(clone->sock, buf, 4, 0) < 4)
     {
          if (conf.debug)
          {
               print(1, 2, 0, "[%s;%s] SOCKS5: Permission denied", clone->nick, clone->proxy);
          }
          return EXIT;
     }

     if (buf[0] != 5 && buf[1] != 0)
     {
          return EXIT;
     }

     if (buf[3] == 1)
     {
          if (recv(clone->sock, buf, 6, 0) != 6)
          {
               return EXIT;
          }
     }
     else if (buf[3] == 4)
     {
          if (recv(clone->sock, buf, 18, 0) != 18)
          {
               return EXIT;
          }
     }
     else if (buf[3] == 3)
     {
          if (recv(clone->sock, buf, 1, 0) != 1)
          {
               return EXIT;
          }
          packetlen = buf[0] + 2;
          if (recv(clone->sock, buf, packetlen, 0) != packetlen)
          {
               return EXIT;
          }
     }

     if (conf.debug)
     {
          print(1, 2, 0, "[%s;%s] SOCKS5: Success", clone->nick, clone->proxy);
     }
     return WAIT_IDENT;
}
/* }}} */
