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
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include "../include/muhstik.h"
#include "../include/clone.h"
#include "../include/proxy.h"
#include "../include/control.h"
#include "../include/print.h"
#include "../include/load.h"
#include "../include/string.h"
/* }}} */
/* {{{ Variables */
/* {{{ External variables */
extern clone_t *cl[];
extern const char *strtype[];
extern char *channel[];
extern pthread_mutex_t mutex[];
extern pthread_attr_t attr;
extern int mass_ch;
extern int maxsock;
extern config_t conf;
extern char *hostname;
/* }}} */
/* {{{ Global Variables */
time_t t0;
pid_t pid;
/* }}} */
/* }}} */
/* {{{ save_list() */
void save_list(char *filename, char **list, int max)
{
     int i;
     FILE *f;

     if ((f = fopen(filename, "w")))
     {
          for (i = 0; i < max; ++i)
          {
               if (list[i])
               {
                    fprintf(f, "%s\n", list[i]);
               }
          }
          fclose(f);
     }
}
/* }}} */
/* {{{ main() functions */
void main_exit()
{
     if (conf.userlist[AOP])
     {
          save_list(conf.userlist[AOP], conf.aop, MAX_AOPS);
     }
     if (conf.userlist[JUPE])
     {
          save_list(conf.userlist[JUPE], conf.jupe, MAX_JUPES);
     }
     if (conf.userlist[PROT])
     {
          save_list(conf.userlist[PROT], conf.prot, MAX_PROTS);
     }
     if (conf.userlist[SHIT])
     {
          save_list(conf.userlist[SHIT], conf.shit, MAX_SHITS);
     }

     exit(EXIT_SUCCESS);
}

void segfault(int sig)
{
     puts("Segmentation fault: blog it.");
     exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
     pthread_t tid;

     printf("muhstik %s, Copyright (C) 2001-2003, Louis Bavoil\n", VERSION);
     printf("               Copyright (C) 2009-2011, Leon Kaiser\n");
     printf("This is free software. See the LICENSE file for more details.\n");

     if (argc < 2)
     {
          printf("Usage: %s <config file>\n", argv[0]);
          exit(EXIT_FAILURE);
     }

     signal(SIGTTIN, SIG_IGN);
     signal(SIGTTOU, SIG_IGN);
     signal(SIGALRM, SIG_IGN);
     signal(SIGPIPE, SIG_IGN);
     signal(SIGUSR1, SIG_IGN);
     signal(SIGSEGV, &segfault);
     signal(SIGINT, SIG_IGN);

     t0 = time(NULL);
     pid = getpid();
     srandom(pid);

     init_options(argv);
     check_options();
     init_threads();
     init_hostname();

     if (conf.motd)
     {
          print_motd(1);
     }
     if (conf.help)
     {
          usage(1);
     }

     if (pthread_create(&tid, &attr, load_all, NULL))
     {
          puts("pthread_create() failed");
          main_exit();
     }

     if (conf.batch)
     {
          read_batch();
     }

     main_loop();

     return 0;
}
/* }}} */
/* {{{ read_batch() */
void read_batch()
{
     FILE *f;
     char buffer[BIGBUF];

     if (!(f = fopen(conf.batch, "r")))
     {
          print(1, 0, 0, "Cannot open the file `%s.'", conf.batch);
          return;
     }
     while (1)
     {
          if (!fgets(buffer, sizeof(buffer), f))
          {
               return;
          }
          interpret(buffer, 0);
     }
     fclose(f);
}
/* }}} */
/* {{{ sock2clone() */
clone_t *sock2clone(int sock)
{
     register int i;
     clone_t **pcl;

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if (*pcl && (*pcl)->sock == sock)
          {
               return *pcl;
          }
     }

     return NULL;
}
/* }}} */
/* {{{ init_gateway() */
int init_gateway(clone_t *clone)
{
     int ret = EXIT;

     clone->host = clone->server;
     clone->port = clone->server_port;

     switch (clone->type)
     {
          case SOCKS4:
               ret = init_socks4(clone);
               break;
          case SOCKS5:
               ret = init_socks5(clone);
               break;
          case PROXY:
               ret = init_proxy(clone);
               break;
          case CISCO:
               ret = WAIT_CISCO;
               break;
     }
     return ret;
}
/* }}} */
/* {{{ Read/write functions */
void write_ready(int sok)
{
     clone_t *clone;
     unsigned int len = sizeof(int);

     if ((clone = sock2clone(sok)))
     {
          if (getsockopt(sok, SOL_SOCKET, SO_ERROR, &errno, &len))
          {
               print_error("getsockopt");
               clone->status = EXIT;
               return;
          }
          if (errno != 0)
          {
               print_error("connect");
               clone->status = EXIT;
               return;
          }
          if (clone->type != NOSPOOF)
          {
               print(1, 0, 0, "%s connect()ed: host=%s server=%s", strtype[clone->type], clone->proxy, clone->server);
          }
          else
          {
               print(1, 0, 0, "direct connect()ed: server=%s", clone->server);
          }

          if (clone->type == NOSPOOF || clone->type == VHOST)
          {
               clone->status = init_irc(clone);
          }
          else
          {
               clone->status = init_gateway(clone);
          }

          return;
     }
}

int read_irc(clone_t *clone)
{
     char *buf;
     char *line;
     int i;
     int n;

     i = strlen(clone->buffer);
     n = sizeof(clone->buffer) - 1 - i;

     if (recv(clone->sock, clone->buffer + i, n, 0) <= 0)
     {
          buf = clone->lastbuffer;
          if (buf[strlen(buf)-1] != '\n')
          {
               StrCat(buf, "\n", sizeof(clone->lastbuffer));
          }
          if (parse_deco(clone, buf))
          {
               return EXIT;
          }
          return WAIT_CONNECT;
     }
     else
     {
          buf = clone->buffer;
          while (strchr(buf,'\n'))
          {
               line = strsep(&buf, "\r\n");
               if (buf == NULL)
               {
                    buf = "";
               }
               snprintf(clone->lastbuffer, sizeof(clone->lastbuffer), "%s\n", line);
               parse_irc(clone, clone->lastbuffer);
          }
          StrCopy(clone->lastbuffer, clone->buffer, sizeof(clone->lastbuffer));
          StrCopy(clone->buffer, buf, sizeof(clone->buffer));
     }
     if (conf.no_restricted && clone->restricted)
     {
          return EXIT;
     }
     return WAIT_IRC;
}

void read_stdin()
{
     char buffer[BIGBUF];

     memset(buffer, 0, sizeof(buffer));
     if (!fgets(buffer, sizeof(buffer), stdin))
     {
          return;
     }

     interpret(buffer, 1);
}

void read_ready(int sok)
{
     clone_t *clone;
     int ret = EXIT;

     if (sok == 0)
     {
          read_stdin();
          return;
     }

     if ((clone = sock2clone(sok)))
     {
          switch (clone->status)
          {
               case WAIT_SOCKS4:
                    ret = read_socks4(clone);
                    break;
               case WAIT_SOCKS5_1:
                    ret = init_read_socks5(clone);
                    break;
               case WAIT_SOCKS5_2:
                    ret = read_socks5(clone);
                    break;
               case WAIT_PROXY:
                    ret = read_proxy(clone);
                    break;
               case WAIT_CISCO:
                    ret = read_cisco(clone);
                    break;
               case WAIT_IRC:
                    ret = read_irc(clone);
                    break;
          }

          if (ret == WAIT_IDENT)
          {
               ret = init_irc(clone);
          }

          clone->status = ret;
          return;
     }
}
/* }}} */
/* {{{ fill_fds() */
void fill_fds(fd_set *rfds, fd_set *wfds)
{
     clone_t **pcl;
     register int i;
     time_t now;

     now = time(NULL);
     FD_SET(0, rfds);

     for (i = 0, pcl = cl; i < MAX_CLONES; ++i, ++pcl)
     {
          if (*pcl)
          {
               if ((*pcl)->rejoin_time > 0)
               {
                    if (!channel[mass_ch])
                    {
                         (*pcl)->rejoin_time = 0;
                    }
                    else if ((*pcl)->rejoin_time <= now)
                    {
                         join(*pcl, channel[mass_ch]);
                         (*pcl)->rejoin_time = 0;
                    }
               }
               if ((*pcl)->alarm > 0)
               {
                    if ((*pcl)->alarm > now)
                    {
                         continue;
                    }
                    (*pcl)->alarm = 0;
               }
               switch ((*pcl)->status)
               {
                    case EXIT:
                         /* Connection closed */
                         free_clone(*pcl);
                         break;
                    case WAIT_CONNECT:
                         if (now-(*pcl)->start > conf.timeout)
                         {
                              /* Connection timeout */
                              free_clone(*pcl);
                              break;
                         }
                         FD_SET((*pcl)->sock, wfds);
                         break;
                    default:
                         FD_SET((*pcl)->sock, rfds);
                         break;
               }
          }
     }
}
/* }}} */
/* {{{ main_loop() */
void main_loop()
{
     fd_set rfds;
     fd_set wfds;
     register int sok;
     struct timeval tv;

     while (1)
     {
          FD_ZERO(&rfds);
          FD_ZERO(&wfds);

          pthread_mutex_lock(&mutex[0]);
          fill_fds(&rfds, &wfds);
          pthread_mutex_unlock(&mutex[0]);

          tv.tv_sec = 1;
          tv.tv_usec = 0;

          if (select(maxsock + 1, &rfds, &wfds, NULL, &tv) == -1)
          {
               print_error("select");
               main_exit();
          }

          for (sok = 0; sok <= maxsock; ++sok)
          {
               if (FD_ISSET(sok, &wfds))
               {
                    pthread_mutex_lock(&mutex[0]);
                    write_ready(sok);
                    pthread_mutex_unlock(&mutex[0]);
               }
          }

          for (sok = maxsock; sok >= 0; --sok)
          {
               if (FD_ISSET(sok, &rfds))
               {
                    pthread_mutex_lock(&mutex[0]);
                    read_ready(sok);
                    pthread_mutex_unlock(&mutex[0]);
               }
          }
     }
}
/* }}} */
