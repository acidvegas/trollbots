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

#ifndef MASS_H
#define MASS_H

#include "clone.h"

enum mass_actions {
     MKB,               /* Mass kickban */
     MK,                /* Mass kick */
     MD,                /* Mass deop */
     TO                 /* Takeover */
};

void massdeop (clone_t *clone, int chid);
void massop (clone_t *clone, int chid);
void force_massop ();
void *init_massdo (int chid, int mode);

#endif
