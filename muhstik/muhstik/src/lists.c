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

#include "../include/lists.h"
#include "../include/string.h"
/* }}} */
/* {{{ File functions */
/* {{{ int occur_file(char *s, char *list) */
int occur_file(char *s, char *list)
{
     char buffer[BIGBUF];
     register int i = 0;
     FILE *f;

     if ((f = fopen(list, "r")))
     {
          while (fgets(buffer, sizeof(buffer), f))
          {
               if (!StrCompare(buffer, s))
               {
                    ++i;
               }
          fclose(f);
          }
     }

     return i;
}
/* }}} */
/* {{{ void add_file(char *s, char *list) */
void add_file(char *s, char *list)
{
     FILE *f;

     if (!(f = fopen(list, "a+")))
     {
          return;
     }
     fputs(s, f);
     fclose(f);
}
/* }}} */
/* {{{ int file_length(FILE *f) */
int file_length(FILE *f)
{
     register int i = 0;
     int c = 0;

     while ((c=fgetc(f)) != EOF)
     {
          if (c == '\n')
          {
               ++i;
          }
     }
     rewind(f);
     return i;
}
/* }}} */
/* {{{ int get_a_line(char *s, int z, FILE *f) */
int get_a_line(char *s, int z, FILE *f)
{
     do
     {
          if (!fgets(s, z, f))
          {
               return 1;
          }
     }
     while (s[0]=='#' || s[0]=='[' || s[0]==10);
     return 0;
}
/* }}} */
/* }}} */
/* {{{ Table functions */
/* {{{ int add_table(char *toadd, char **list, int max) */
int add_table(char *toadd, char **list, int max)
{
     register int i;
     for (i = 0; i < max; ++i, ++list)
     {
          if (*list == NULL)
          {
               *list = StrDuplicate(toadd);
               return i;
          }
     }
     return -1;
}
/* }}} */
/* {{{ int remove_table(char *torm, char **list, int max) */
int remove_table(char *torm, char **list, int max)
{
     register int i;
     for (i = 0; i < max; ++i, ++list)
     {
          if (*list && !StrCompare(*list, torm))
          {
               free(*list);
               *list = NULL;
               return 0;
          }
     }
     return 1;
}
/* }}} */
/* {{{ void update_table(char *from, char *to, char **list, int max) */
void update_table(char *from, char *to, char **list, int max)
{
     register int i;
     for (i = 0; i < max; ++i, ++list)
     {
          if (*list && !StrCompare(from, *list))
          {
               free(*list);
               *list = StrDuplicate(to);
               return;
          }
     }
}
/* }}} */
/* {{{ void update_pattern_table(char *from, char *to, char **list, int max) */
void update_pattern_table(char *from, char *to, char **list, int max)
{
     register int i;
     char buffer[MEDBUF+1];

     for (i = 0; i < max; ++i, ++list)
     {
          if ((*list) && (sscanf(*list, "%"MEDBUF_TXT"[^!]", buffer) == 1) && (!StrCompare(from, buffer)))
          {
               snprintf(buffer, sizeof(buffer), "%s%s", to, strchr(*list, '!'));
               free(*list);
               *list = StrDuplicate(buffer);
               return;
          }
     }
}
/* }}} */
/* {{{ int occur_table(char *s, char **list, int max) */
int occur_table(char *s, char **list, int max)
{
     register int i;
     int k = 0;
     for (i = 0; i < max; ++i, ++list)
     {
          if (*list && !StrCompare(*list, s))
          {
               ++k;
          }
     }
     return k;
}
/* }}} */
/* {{{ void fill_table(char **list, int max, FILE *f) */
void fill_table(char **list, int max, FILE *f)
{
     register int i;
     char parm[MEDBUF];
     for (i = 0; i < max && fgets(parm, sizeof(parm), f); ++i)
     {
          sscanf(parm, "%[^\r\n]", parm);
          list[i] = StrDuplicate(parm);
     }
}
/* }}} */
/* {{{ void clear_table(char **list, int max) */
void clear_table(char **list, int max)
{
     register int i;
     for (i = 0; i < max; ++i, ++list)
     {
          if (*list)
          {
               free(*list);
               *list = NULL;
          }
     }
}
/* }}} */
/* {{{ int match_table(char *s, char **list, int max) */
int match_table(char *s, char **list, int max)
{
     register int i;
     char parm[MEDBUF];

     for (i = 0; i < max; ++i, ++list)
     {
          if (*list && !StrParam(parm, sizeof(parm), *list, 0))
          {
               if (match_pattern(parm, s))
               {
                    return i;
               }
          }
     }

     return -1;
}
/* }}} */
/* }}} */
/* {{{ Queue functions */
/* {{{ int add_gen_queue(void *d, cmp_fun *cmp, queue *list) */
int add_gen_queue(void *d, cmp_fun *cmp, queue *list)
{
     queue s;
     queue p1 = NULL;
     queue p2 = NULL;

     s = xmalloc(sizeof(struct st_list));
     s->data = d;
     s->next = NULL;

     if (*list == NULL)
     {
          *list = s;
          return 0;
     }

     for (p2 = *list; p2; p2 = p2->next)
     {
          if (cmp && !cmp(p2->data, d))
          {
               free(s->data);
               free(s);
               return 1;
          }
          p1 = p2;
     }
     p1->next = s;
     return 0;
}
/* }}} */
/* {{{ inline int add_queue(char *s, queue *list) */
inline int add_queue(char *s, queue *list)
{
     return add_gen_queue(StrDuplicate(s), NULL, list);
}
/* }}} */
/* {{{ inline int uniq_add_queue(char *s, queue *list) */
inline int uniq_add_queue(char *s, queue *list)
{
     return add_gen_queue(StrDuplicate(s), StrCompare, list);
}
/* }}} */
/* {{{ int remove_queue(char *d, queue *list) */
int remove_queue(char *d, queue *list)
{
     queue p1 = NULL;
     queue p2 = NULL;

     if (*list == NULL)
     {
          return 1;
     }

     if (!StrCompare((*list)->data, d))
     {
          free_cell(list);
          return 0;
     }

     for (p2 = *list; p2; p2 = p2->next)
     {
          if (!StrCompare(p2->data, d))
          {
               p1->next = p2->next;
               free(p2->data);
               free(p2);
               return 0;
          }
          p1 = p2;
     }
     return 1;
}
/* }}} */
/* {{{ void update_queue(char *old, char *new, queue *list) */
void update_queue(char *old, char *new, queue *list)
{
     queue p;

     for (p = *list; p; p = p->next)
     {
          if (!StrCompare(p->data, old))
          {
               free(p->data);
               p->data = StrDuplicate(new);
          }
     }
}
/* }}} */
/* {{{ int occur_queue(char *s, queue *list) */
int occur_queue(char *s, queue *list)
{
     queue p;
     int k = 0;

     for (p = *list; p; p = p->next)
     {
          if (!StrCompare(p->data, s))
          {
               ++k;
          }
     }

     return k;
}
/* }}} */
/* }}} */
/* {{{ Cell functions */
/* {{{ void free_cell(queue *list) */
void free_cell(queue *list)
{
     queue tmp;

     if (*list == NULL)
     {
          return;
     }

     tmp = (*list)->next;
     free((*list)->data);
     free(*list);
     *list = tmp;
}
/* }}} */
/* {{{ void rotate_cell(queue *list) */
void rotate_cell(queue *list)
{
     queue cell;
     void *data;

     cell = (*list)->next;
     data = (*list)->data;
     free(*list);
     *list = cell;
     add_gen_queue(data, NULL, list);
}
/* }}} */
/* }}} */
/* {{{ void clear_queue(queue *list) */
void clear_queue(queue *list)
{
     while (*list)
     {
          free_cell(list);
     }
}
/* }}} */
