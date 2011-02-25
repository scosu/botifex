//    botifex is a communication bot for irc networks learning from conversations
//    Copyright (C) 2011  Markus Pargmann <mpargman_AT_allfex_DOT_org>
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef TRIE_H_
#define TRIE_H_

#include <stdlib.h>
#include <glib.h>

typedef struct
{
	char *ind;
	void *data;
	GSList *childs;
} trie_element_t;

typedef struct
{
	trie_element_t* root;
} trie_t;


trie_t *trie_init();

int trie_load_file(trie_t *t, const char *path);

int trie_save_file(trie_t *t, const char *path);

void *trie_add(trie_t *t, const char *ind, const size_t size);

void trie_del(trie_t *t, const char *ind);

void *trie_get(trie_t *t, const char *ind);


#endif /* TRIE_H_ */
