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

#ifndef STRING_H
#define STRING_H

#include <string.h>
#include <errno.h>
extern int errno;

/* Functions for handling strings */

/* A delimiter for strsep */
#define DELIM " \r\n"

void *xmalloc (size_t size);
char *StrDuplicate (char *src);
int StrCompare (char *s1, char *s2);
int StrCmpPrefix (char *s1, char *s2);
void StrFirstToken (char *s);
void StrCopy (char *s1, char *s2, size_t n);
void StrCat (char *s1, char *s2, size_t n);
int StrParam (char *parm, size_t size, char *s, int i);
int is_in (char *w, char *s);

#ifdef NO_STRSEP
char *strsep (char **stringp, const char *delim);
#endif

/* Functions for handling patterns */

int is_pattern (char *s);
int is_nick (char *s);
int match_pattern (char *pattern, char *s);

#endif
