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

#ifndef LOAD_H
#define LOAD_H

#include "clone.h"

enum load_mode {
     M_QUIT,
     M_NOJOIN,
     M_NORMAL
};

void *load_all (void *arg);
int load_host (int t, char *host, int proxy_port, char *server, int server_port, char *pass, char *ident, char *save, int mode);
void randget (clone_t *clone, char **dest, size_t destlen, int uselist, char **list, int max);

#endif
