/* Muhstik, Copyright (C) 2001-2002, Louis Bavoil <mulder@gmx.fr>       */
/*          Copyright (C) 2009-2011, Leon Kaiser <literalka@gnaa.eu>    */
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

#ifndef MUHSTIK_H
#define MUHSTIK_H

#include "clone.h"

void read_batch ();
void read_stdin ();
int init_irc (clone_t *clone);
void main_exit ();
void main_loop ();

#endif
