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
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/string.h"
#include "../include/lists.h"
#include "../include/print.h"
#include "../include/load.h"
/* }}} */
/* {{{ External variables */
extern clone_t *cl[];
extern config_t conf;
extern const char *strtype[];
extern pthread_mutex_t mutex[];
/* }}} */
/* {{{ randget() */
void randget(clone_t *clone, char **dest, size_t destlen, int uselist, char **list, int max)
{
     register unsigned int i; /* ``unsigned'' is not needed, stops a warning when compiled with -Wsign-compare */
     char *tmp;

     /* Alphabet used to make random words */
     static char charset[] = "abcdefghijklmnopqrstuvwxyz";
     static size_t len;

     len = strlen(charset);
     tmp = (char *)xmalloc(destlen + 1);

     if (uselist && list[0])
     {
          for (i = 0; i < MAX_NICKS && list[i]; ++i) {}
          StrCopy(tmp, list[random() % i], destlen + 1);
     }
     else
     {
          for (i = 0; i < destlen; ++i)
          {
               tmp[i] = charset[random() % len];
          }
     }
     tmp[destlen] = 0;
     *dest = tmp;
}
/* }}} */
/* {{{ Loading functions */
int load_clone(clone_t *go)
{
     clone_t **pcl;
     register int id;

     /* Get a free entry in cl[] */
     for (id = 0, pcl = cl; *pcl; ++id, ++pcl)
     {
          if (id == MAX_CLONES-1)
          {
               return 1;
          }
     }

     /* Allocate memory for a new clone structure */
     if (!(*pcl = (clone_t *)malloc(sizeof(clone_t))))
     {
          print_error("malloc");
          return 1;
     }

     /* Copy the current clone parameters in it */
     memcpy(*pcl, go, sizeof(clone_t));

     /* The id is useful to free the structure on exit */
     (*pcl)->id = id;

     /* Launch the clone */
     if (main_clone(*pcl))
     {
          free_clone(*pcl);
     }

     return 0;
}

int load_host(int t, char *host, int proxy_port, char *server, int server_port, char *pass, char *ident, char *save, int mode)
{
     clone_t go;

     /* Initialize the current clone structure */
     memset(&go, 0, sizeof(clone_t));
     memset(&go.buffer, 0, sizeof(go.buffer));
     memset(&go.lastbuffer, 0, sizeof(go.lastbuffer));
     go.sock = -1;
     go.type = t;
     go.mode = mode;

     if (go.type != NOSPOOF)
     {
          go.proxy = StrDuplicate(host);
          go.proxy_port = proxy_port;
     }

     go.server = StrDuplicate(server);
     go.server_port = server_port;
     go.server_pass = StrDuplicate(pass);
     go.server_ident = StrDuplicate(ident);
     go.save = StrDuplicate(save);

     /* Make the clone's {nick,ident,realname} from a wordlist or randomly. */
     randget(&go, &go.nick, conf.nick_length, conf.use_wordlist, conf.nicks, MAX_NICKS);
     randget(&go, &go.ident, conf.ident_length, 1, conf.idents, MAX_IDENTS);
     randget(&go, &go.real, conf.real_length, 1, conf.names, MAX_NAMES);

     return load_clone(&go);
}

void loadlist(int t, FILE *hosts, FILE *servers, char *save)
{
     int ret;
     int i = 0;
     int k = 0;
     int l = 0;
     int max_server = 0;
     int max_proxys = 0;
     char server[MEDBUF+1];
     char *serv = NULL;
     char *pass = NULL;
     char *ident = NULL;
     int server_port;
     char proxy[MEDBUF+1];
     int proxy_port;
     char buffer[MEDBUF+1];
     char parm[MINIBUF];

     if (!conf.wait_socks && hosts)
     {
          max_proxys = file_length(hosts);
     }

     while (1)
     {
          if (i == 0)
          {
               /* Get the next server on the current server list */
               if (!fgets(buffer, sizeof(buffer), servers))
               {
                    return;
               }
               switch (sscanf(buffer, "%"MEDBUF_TXT"[^:\r\n]:%d %d", server, &server_port, &max_server))
               {
                    default:
                         print(1, 0, 0, "No line to read");
                         return;
                    case 1:
                         print(1, 0, 0, "%s: server error (host:port)", server);
                         return;
                    case 2:
                         print(1, 0, 0, "%s: server error (no max)", server);
                         return;
                    case 3:
                         print(1, 3, 0, "%s server: %s:%d [max=%d]", strtype[t], server, server_port, max_server);

                         if (!StrParam(parm, sizeof(parm), buffer, 2))
                         {
                              pass = StrDuplicate(parm);
                         }

                         if (!StrParam(parm, sizeof(parm), buffer, 3))
                         {
                              ident = StrDuplicate(parm);
                         }

                         if (t != NOSPOOF && conf.rewind_socks)
                         {
                              rewind(hosts);
                         }

                         break;
               }
               if (t == NOSPOOF && max_server <= 0)
               {
                    print(1, 0, 0, "%s: server error (max must be > 0)", server);
                    return;
               }
               serv = StrDuplicate(server);
          }

          /* Get the next proxy on the current proxy list, if needed */
          if (t != NOSPOOF && k == 0)
          {
               while (get_a_line(buffer, sizeof(buffer), hosts))
               {
                    if (conf.wait_socks)
                    {
                         sleep(2);
                    }
                    else
                    {
                         return;
                    }
               }
               sscanf(buffer, "%"MEDBUF_TXT"s", proxy);
               if (t != VHOST)
               {
                    if (strchr(proxy,':'))
                    {
                         sscanf(proxy, "%*[^:]:%d", &proxy_port); /* XXX: What if ``(t == VHOST)''? -- Leon */
                         sscanf(proxy, "%[^:]", proxy);
                    }
                    else switch (t) /* set a default port */
                    {
                         case CISCO:
                              proxy_port = 23;
                              break;
                         case SOCKS4:
                         case SOCKS5:
                              proxy_port = 1080;
                              break;
                         case PROXY:
                              print(1, 0, 0, "%s: proxy error (host:port)", proxy);
                              proxy_port = 8080;
                              break;
                    }
               }
          }

          while ((max_server <= 0 || i < max_server) && (t == NOSPOOF || k < conf.max_clones))
          {
               pthread_mutex_lock(&mutex[0]);
               ret = load_host(t, proxy, proxy_port, serv, server_port, pass, ident, save, M_NORMAL);
               pthread_mutex_unlock(&mutex[0]);
               if (ret)
               {
                    pthread_exit(0);
               }

               ++i, ++k, ++l;

               if (l == conf.group && conf.load > 0)
               {
                    usleep(conf.load * 1000), l = 0;
               }
          }

          if (i == max_server)
          {
               i = 0;
          }
          if (k == conf.max_clones)
          {
               k = 0;
          }

          if (!conf.wait_socks && hosts && max_server <= 0)
          {
               if (i == max_proxys * conf.max_clones)
               {
                    i = 0;
               }
          }
     }
}

void load1(int t, FILE *hosts, FILE *servers, char *save)
{
     loadlist(t, hosts, servers, save);
     if (conf.verbose)
     {
          print(1, 0, 0, "all %ss loaded.", strtype[t]);
     }
}

void load2(int t, FILE **p1, FILE **p2, char **p3)
{
     FILE **fhost;
     FILE **fserv;
     char **csave;
     for (fhost = p1, fserv = p2, csave = p3; *fhost; ++fhost, ++fserv, ++csave)
     {
          load1(t, *fhost, *fserv, *csave);
     }
}

void *load_all(void *arg)
{
     int i;

     for (i = 0; i < PROXYS; ++i)
     {
          if (conf.h[i][0])
          {
               load2(i, conf.h[i], conf.s[i], conf.g[i]);
          }
     }

     if (conf.direct[0])
     {
          load1(NOSPOOF, NULL, conf.direct[0], NULL);
     }

     return NULL;
}
/* }}} */
