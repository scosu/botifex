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

#include "trie.h"
#include <glib.h>
#include <string.h>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

trie_t *trie_init()
{
	trie_t *t = malloc(sizeof(trie_t));
	t->root->data = NULL;
	t->root->ind = NULL;
	t->root->childs = NULL;
	return t;
}

//int trie_load_file(trie_t* t, const char path);

//int trie_save_file(trie_t* t, const char path);

static inline trie_element_t* trie_element_match(GSList* list, const char** word_ind, int* most_match)
{
	if(unlikely(list == NULL))
	{
		return NULL;
	}
	GSList* cur = list;
	const char* word = *word_ind;
	int no_match = 1;
	int ind;
	most_match = 0;
	do
	{
		const char* element = ((trie_element_t*)cur->data)->ind;

		for(ind = 0; element[ind] == word[ind]; ++ind){no_match = 0;}
		if(element[ind] == '\0')
		{
			word_ind += ind;
			return (trie_element_t*)cur->data;
		}
	}
	while(likely(no_match && (cur = g_slist_next(cur)) != NULL));
	if(most_match != NULL)
	{
		*most_match = ind;
	}
	return (trie_element_t*)cur->data;
}

void* trie_add(trie_t* t, const char* ind, const size_t size)
{
	trie_element_t* cur = t->root;
	const char* i = ind;
	while(1)
	{
		int matchct;
		trie_element_t* result = trie_element_match(cur->childs, &i, &matchct);
		if(unlikely(result == NULL))
		{
			trie_element_t* tmp = malloc(sizeof(trie_element_t));
			tmp->ind = malloc((i-ind)*sizeof(char) + 1);
			tmp->data = malloc(size);
			tmp->childs = NULL;

			result->childs = g_slist_prepend(result->childs, tmp);

			return tmp->data;
		}
		if(unlikely(matchct != 0))
		{
			// first split the current existing edge
			int moved_length = strlen(result->ind);
			char* moved = malloc(sizeof(char) * moved_length+1);
			char* to_current = malloc(sizeof(char) * matchct+1);
			strncpy(to_current, result->ind, matchct);
			to_current[matchct-1] = '\0';
			strcpy(moved, result->ind + matchct);
			free(result->ind);
			result->ind = to_current;
			trie_element_t* tmp = malloc(sizeof(trie_element_t));
			tmp->childs = result->childs;
			tmp->data = result->data;
			result->data = NULL;
			tmp->ind = moved;
			result->childs = NULL;
			result->childs = g_slist_prepend(result->childs, tmp);
			tmp = malloc(sizeof(trie_element_t));
			tmp->childs = NULL;
			tmp->ind = malloc(strlen(i + matchct) * sizeof(char) + 1);
			strcpy(tmp->ind, i);
			return tmp->data = malloc(size);
		}
		cur = result;
		if(unlikely(*i != '\0'))
		{
			return cur->data;
		}
	}
	return NULL;
}

void trie_del(trie_t* t, const char* ind)
{
	trie_element_t* cur = t->root;
	while(1)
	{
		cur = trie_element_match(cur->childs, &ind, NULL);
		if(unlikely(cur == NULL))
		{
			return;
		}
		if(unlikely(*ind != '\0'))
		{

		}
	}
	return;
}

void* trie_get(trie_t* t, const char* ind)
{
	trie_element_t* cur = t->root;
	while(1)
	{
		cur = trie_element_match(cur->childs, &ind, NULL);
		if(unlikely(cur == NULL))
		{
			return NULL;
		}
		if(unlikely(*ind != '\0'))
		{
			return cur->data;
		}
	}
	return NULL;
}

