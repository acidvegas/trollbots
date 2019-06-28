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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/init.h"
#include "../include/string.h"
#include "../include/lists.h"
#include "../include/print.h"
/* }}} */
/*{{{ Global variables */
char *hostname;
config_t conf;
char *channel[MAX_CHANS];
char *chankey[MAX_CHANS];
pthread_attr_t attr;
pthread_mutex_t mutex[MAX_MUTEX];
/* }}} */
/* {{{ check_options() */
void check_options()
{
     int i;

     for (i = 0; i < PROXYS && !conf.h[i][0]; ++i);
     {
          if (i == PROXYS && !conf.direct[0])
          {
               puts("You must supply a way of connection (proxy, socks, etc).");
               exit(EXIT_FAILURE);
          }
     }

     for (i = 0; i < PROXYS; ++i)
     {
          if (conf.h[i][0] && !conf.s[i][0])
          {
               puts("You must supply a server list for each proxy list.");
               exit(EXIT_FAILURE);
          }
     }

     for (i = 0; i < PROXYS; ++i)
     {
          if (!conf.h[i][0] && conf.s[i][0])
          {
               puts("You must supply a proxy list for each server list.");
               exit(EXIT_FAILURE);
          }
     }

     if (conf.use_wordlist && !conf.nicks[0])
     {
          puts("No nicknames to use.");
          exit(EXIT_FAILURE);
     }
}
/* }}} */
/* {{{ init_*() */
void init_threads()
{
     int i;

     if (pthread_attr_init(&attr))
     {
          puts("pthread_attr_init failed");
          exit(EXIT_FAILURE);
     }
     if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
     {
          puts("pthread_attr_setdetachstate failed");
          exit(EXIT_FAILURE);
     }

     for (i = 0; i < MAX_MUTEX; ++i)
     {
          if (pthread_mutex_init(&mutex[i], NULL))
          {
               puts("pthread_mutex_init failed");
               exit(EXIT_FAILURE);
          }
     }
}

void init_hostname()
{
     char name[MEDBUF];

     if (gethostname(name, sizeof(name)) == -1)
     {
          print_error("gethostname");
          exit(EXIT_FAILURE);
     }
     StrFirstToken(name);
     hostname = StrDuplicate(name);
}

void init_options(char **argv)
{
     FILE *config, *f;
     unsigned i;
     unsigned j;
     char buffer[MEDBUF];
     char parm[MEDBUF];

     struct
     {
          char *arg;
          int *ptr;
     } init_int[] = {
          { "load",             &conf.load            },
          { "group",            &conf.group           },
          { "clones",           &conf.max_clones      },
          { "wait",             &conf.wait_socks      },
          { "rewind",           &conf.rewind_socks    },
          { "timeout",          &conf.timeout         },
          { "max_reco",         &conf.max_reco        },
          { "wait_reco",        &conf.wait_reco       },
          { "no_restricted",    &conf.no_restricted   },
          { "scan",             &conf.scan            },
          { "help",             &conf.help            },
          { "verbose",          &conf.verbose         },
          { "debug",            &conf.debug           },
          { "nocolor",          &conf.nocolor         },
          { "dalnet",           &conf.dalnet          },
          { "using_wordlist",   &conf.use_wordlist    },
          { "nick_length",      &conf.nick_length     },
          { "ident_length",     &conf.ident_length    },
          { "real_length",      &conf.real_length     },
          { "rejoin",           &conf.rejoin          },
          { "multi_op",         &conf.multi_op        },
          { "multi_kick",       &conf.multi_kick      },
          { "multi_deop",       &conf.multi_deop      },
          { "aggressive",       &conf.aggressive      },
          { "peace",            &conf.peace           },
          { "repeat",           &conf.repeat          },
          { "notice",           &conf.notice          },
          /* { "max_conns",        &conf.max_conns       }, */
          { NULL,               NULL                  }
     };

     struct
     {
          char *arg;
          FILE **ptr;
          short min;
          short max;
     } init_list[] = {
          { "sock4_list",             conf.h[SOCKS4],         0,      MAX_GROUPS   },
          { "sock4_server_list",      conf.s[SOCKS4],         0,      MAX_GROUPS   },
          { "sock5_list",             conf.h[SOCKS5],         0,      MAX_GROUPS   },
          { "sock5_server_list",      conf.s[SOCKS5],         0,      MAX_GROUPS   },
          { "proxy_list",             conf.h[PROXY],          0,      MAX_GROUPS   },
          { "proxy_server_list",      conf.s[PROXY],          0,      MAX_GROUPS   },
          { "cisco_list",             conf.h[CISCO],          0,      MAX_GROUPS   },
          { "cisco_server_list",      conf.s[CISCO],          0,      MAX_GROUPS   },
          { "vhost_list",             conf.h[VHOST],          0,      MAX_GROUPS   },
          { "vhost_server_list",      conf.s[VHOST],          0,      MAX_GROUPS   },
          { "direct_server_list",     conf.direct,            0,      2            },
          { NULL,                     NULL,                   0,      0            }
     };

     struct
     {
          char *arg;
          char **ptr;
          short min;
          short max;
     } init_save[] = {
          { "sock4_save",             conf.g[SOCKS4],         0,      MAX_GROUPS   },
          { "sock5_save",             conf.g[SOCKS5],         0,      MAX_GROUPS   },
          { "proxy_save",             conf.g[PROXY],          0,      MAX_GROUPS   },
          { "vhost_save",             conf.g[VHOST],          0,      MAX_GROUPS   },
          { "cisco_save",             conf.g[CISCO],          0,      MAX_GROUPS   },
          { NULL,                     NULL,                   0,      0            }
     };

     struct
     {
          char *arg;
          char **save;
          char **ptr;
          int max;
     } init_read[] = {
          { "aop_list",       &conf.userlist[AOP],   conf.aop,         MAX_AOPS      },
          { "jupe_list",      &conf.userlist[JUPE],  conf.jupe,        MAX_JUPES     },
          { "prot_list",      &conf.userlist[PROT],  conf.prot,        MAX_PROTS     },
          { "shit_list",      &conf.userlist[SHIT],  conf.shit,        MAX_SHITS     },
          { "nicks",          NULL,                  conf.nicks,       MAX_NICKS     },
          { "idents",         NULL,                  conf.idents,      MAX_IDENTS    },
          { "realnames",      NULL,                  conf.names,       MAX_NAMES     },
          { NULL,             NULL,                  NULL,             0             }
     };

     struct
     {
          char *arg;
          char **ptr;
     } init_str[] = {
          { "onstart",        &conf.batch             },
          { "chan",           channel                 },
          { "motd",           &conf.motd              },
          { NULL,             NULL                    }
     };

     /* Initialize the parameters */
     conf.load = 2000;
     conf.group = 1;
     conf.timeout = 30;
     conf.max_clones = 1;

     conf.multi_kick = 1;
     conf.multi_op = 4;
     conf.multi_deop = 4;

     conf.rejoin = 5;
     conf.dalnet = 1;
     conf.nick_length = 8;
     conf.ident_length = 8;
     conf.real_length = 8;

     if (!(config = fopen(argv[1], "r")))
     {
          puts("Cannot open the config file.");
          exit(EXIT_FAILURE);
     }

     while (!get_a_line(buffer, sizeof(buffer), config))
     {
          if (StrParam(parm, sizeof(parm), buffer, 0))
          {
               continue;
          }

          /* Initialize the integer parameters */
          for (i = 0; init_int[i].arg; ++i)
          {
               if (!StrCompare(init_int[i].arg, parm))
               {
                    if (!StrParam(parm, sizeof(parm), buffer, 1))
                    {
                         *init_int[i].ptr = atoi(parm);
                         break;
                    }
               }
          }
          if (init_int[i].arg)
          {
               continue;
          }

          /* Initialize the file descriptors */
          for (i = 0; init_list[i].arg; ++i)
          {
               if (!StrCompare(init_list[i].arg, parm))
               {
                    if (init_list[i].min < init_list[i].max-1)
                    {
                         if (!StrParam(parm, sizeof(parm), buffer, 1))
                         {
                              j = init_list[i].min++;
                              if (!((init_list[i].ptr)[j] = fopen(parm, "r")))
                              {
                                   printf("Cannot open a %s.\n", init_list[i].arg);
                                   exit(EXIT_FAILURE);
                              }
                              break;
                         }
                    }
               }
          }
          if (init_list[i].arg)
          {
               continue;
          }

          /* Initialize save filenames */
          for (i = 0; init_save[i].arg; ++i)
          {
               if (!StrCompare(init_save[i].arg, parm))
               {
                    if (init_save[i].min < init_save[i].max-1)
                    {
                         if (!StrParam(parm, sizeof(parm), buffer, 1))
                         {
                              j = init_save[i].min++;
                              (init_save[i].ptr)[j] = StrDuplicate(parm);
                              break;
                         }
                    }
               }
          }
          if (init_save[i].arg)
          {
               continue;
          }

          /* Initialize the lists to read */
          for (i = 0; init_read[i].arg; ++i)
          {
               if (!StrCompare(init_read[i].arg, parm))
               {
                    if (!StrParam(parm, sizeof(parm), buffer, 1))
                    {
                         if (init_read[i].save)
                         {
                              *init_read[i].save = StrDuplicate(parm);
                         }
                         if ((f = fopen(parm, "r")))
                         {
                              fill_table(init_read[i].ptr, init_read[i].max, f);
                              fclose(f);
                         }
                         else if (!init_read[i].save)
                         {
                              printf("Cannot open the %s.\n", init_read[i].arg);
                              exit(EXIT_FAILURE);
                         }
                         break;
                    }
               }
          }
          if (init_read[i].arg)
          {
               continue;
          }

          /* Initialize the string parameters */
          for (i = 0; init_str[i].arg; ++i)
          {
               if (!StrCompare(init_str[i].arg, parm))
               {
                    if (!StrParam(parm, sizeof(parm), buffer, 1))
                    {
                         *init_str[i].ptr = StrDuplicate(parm);
                         break;
                    }
               }
          }
          if (init_str[i].arg)
          {
               continue;
          }
     }

     fclose(config);
}
/* }}} */
