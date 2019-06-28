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

#ifndef PRINT_H
#define PRINT_H

#include "clone.h"

/* Functions for handling printing */

#define LEN_LINE 71
#define LEN_TAB 12

void print (int ret, int color, int dest, const char *fmt, ...);
void print_error(char *prefix);
void print_prefix (clone_t *clone, int color, int dest);
void print_line (int out);
void print_motd (int out);
void usage (int out);
void status (int out);

#endif
