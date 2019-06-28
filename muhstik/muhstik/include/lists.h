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

#ifndef LISTS_H
#define LISTS_H

#include "init.h"

/* Functions for handling files */

int occur_file (char *s, char *list);
void add_file (char *s, char *list);
int file_length (FILE *f);
int get_a_line (char *s, int z, FILE *f);

/* Functions for handling tables */

int add_table (char *toadd, char **list, int max);
int remove_table (char *torm, char **list, int max);
void update_table (char *from, char *to, char **list, int max);
void update_pattern_table (char *from, char *to, char **list, int max);
int occur_table (char *s, char **list, int max);
void fill_table (char **list, int max, FILE *f);
void clear_table (char **list, int max);
int match_table (char *s, char **list, int max);

/* Functions for handling single linked lists */

typedef struct st_list *queue;
struct st_list {
     char *data;
     queue next;
};

typedef int cmp_fun (char *s1, char *s2);
int add_queue (char *s, queue *list);
int uniq_add_queue (char *s, queue *list);
int remove_queue (char *d, queue *list);
void update_queue (char *old, char *new, queue *list);
int occur_queue (char *s, queue *list);
void free_cell (queue *list);
void clear_queue (queue *list);
void rotate_cell (queue *list);

#endif
