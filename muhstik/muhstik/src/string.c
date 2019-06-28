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
#include <stdio.h>
#include <stdlib.h>

#include "../include/clone.h"
#include "../include/string.h"
#include "../include/print.h"
/* }}} */
/* {{{ External variables */
extern config_t conf;
/* }}} */
/* {{{ xmalloc() */
void *xmalloc(size_t size)
{
     void *ret;
     if (!(ret = malloc(size)))
     {
          print_error("malloc");
          exit(EXIT_FAILURE);
     }
     return ret;
}
/* }}} */
/* {{{ Str*() */
char *StrDuplicate(char *src)
{
     char *dest;

     if (!src)
     {
          return NULL;
     }

     if (!(dest = strdup(src)))
     {
          print_error("malloc");
          exit(EXIT_FAILURE);
     }
     return dest;
}

inline int StrCompare(char *s1, char *s2)
{
     return (s1 && s2) ? strcasecmp(s1,s2) : 1;
}

inline int StrCmpPrefix(char *s1, char *s2)
{
     return (s1 && s2) ? strncasecmp(s1,s2,strlen(s2)) : 1;
}

inline void StrFirstToken(char *s)
{
     sscanf(s, "%s", s);
}

inline void StrCopy(char *s1, char *s2, size_t n)
{
     strncpy(s1, s2, n);
     s1[n-1] = 0;
}

inline void StrCat(char *s1, char *s2, size_t n)
{
     if (strlen(s1) + strlen(s2) + 1 <= n)
     {
          strcat(s1, s2);
     }
}

#ifdef NO_STRSEP

char *strsep(char **stringp, const char *delim)
{
     char *res;

     if (!stringp || !*stringp || !**stringp)
     {
          return NULL;
     }

     res = *stringp;
     while (**stringp && !strchr(delim, **stringp))
     {
          (*stringp)++;
     }

     if (**stringp)
     {
          **stringp = '\0';
          (*stringp)++;
     }

     return res;
}

#endif

int StrParam(char *parm, size_t size, char *s, int i)
{
     register unsigned int k = 0; /* ``unsigned'' is not needed, stops a warning when compiled with -Wsign-compare */

     memset(parm, 0, size);

     while (i >= 0 && k < size)
     {
          if (*s == 0 || *s == '\n' || *s == '\r')
          {
               break;
          }

          if (*s == ' ')
          {
               --i;
          }
          else if (i == 0)
          {
               parm[k++] = *s;
          }
          ++s;
     }

     return (i > 0 || parm[0] == 0) ? 1 : 0;
}
/* }}} */
/* {{{ Booleans */
int is_in(char *w, char *s)
{
     char *parm;

     while ((parm = strsep(&s, DELIM)))
     {
          if (!StrCompare(parm, w))
          {
               return 1;
          }
     }

     return 0;
}

int is_nick(char *s)
{
    return ((NULL == strchr(s, ',')) && (strlen(s) <= (unsigned) conf.nick_length)); /* ``unsigned'' is not needed, stops a warning when compiled with -Wsign-compare */
}

int is_pattern(char *s)
{
     while ((*s != '\0') && (*s != '!') && (*s != '*'))
     {
          ++s;
     }
     return (*s == '*') || (*s == '!');
}
/* }}} */
/* {{{ match_pattern() */
int match_pattern(char *pattern, char *string)
{
     if (!string || !pattern)
     {
          return 0;
     }

     while (1)
     {
          if (!*string && !*pattern)
          {
               return 1;
          }

          if (!*string || !*pattern)
          {
               return 0;
          }

          if ((*string == *pattern) || (*string && *pattern == '?'))
          {
               ++string;
               ++pattern;
          }
          else if (*pattern == '*')
          {
               if (*string == *(pattern + 1))
               {
                    ++pattern;
               }
               else if (*(pattern + 1) == *(string + 1))
               {
                    ++string;
                    ++pattern;
               }
               else
               {
                    ++string;
               }
          }
          else
          {
               return 0;
          }
     }
}
/* }}} */
