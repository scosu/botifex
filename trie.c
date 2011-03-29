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
	t->root = malloc(sizeof(trie_element_t));
	t->root->data = NULL;
	t->root->ind = NULL;
	t->root->childs = NULL;
	return t;
}

//int trie_load_file(trie_t* t, const char path);

//int trie_save_file(trie_t* t, const char path);



static trie_element_t* trie_element_match_child(GSList* list, const char** word_ind,
		int* most_match)
{
	*most_match = 0;
	if (unlikely(list == NULL)) {
		return NULL;
	}
	GSList *cur = list;
	const char *word = *word_ind;
	int ind;
	while (1) {
		const char* element = ((trie_element_t*) (cur->data))->ind;

		for (ind = 0; element[ind] == word[ind] && element[ind] != '\0'
				&& word[ind] != '\0'; ++ind) {}
		if (element[ind] == '\0') {
			*word_ind += ind;
			return (trie_element_t*) (cur->data);
		}
		if (ind || g_slist_next(cur) == NULL)
			break;
		cur = g_slist_next(cur);
	}
	if (most_match != NULL) {
		*most_match = ind;
	}
	if (!ind)
		return NULL;
	return (trie_element_t*) (cur->data);
}

void trie_del(trie_t* t, const char* ind)
{
	trie_element_t* cur = t->root;
	while (1) {
		cur = trie_element_match_child(cur->childs, &ind, NULL);
		if (unlikely(cur == NULL)) {
			return;
		}
		if (unlikely(*ind != '\0')) {

		}
	}
	return;
}

int trie_get_or_create(trie_t* t, void **data, const char* ind, const size_t size)
{
	trie_element_t* cur = t->root;
	int most_match;
	if (ind[0] == '\0')
		g_critical("trie empty index");
	while (1) {
		trie_element_t *new = trie_element_match_child(cur->childs, &ind, &most_match);
		if (unlikely(new == NULL)) {
			trie_element_t *tmp = malloc(sizeof(trie_element_t));
			tmp->data = malloc(size);
			tmp->childs = NULL;
			tmp->ind = malloc((strlen(ind) + 1) * sizeof(char));
			strcpy(tmp->ind, ind);
			cur->childs = g_slist_prepend(cur->childs, tmp);
			*data = tmp->data;
			if (tmp->ind == '\0')
				g_critical("trie created empty node");
			return 1;
		}
		if (unlikely(*ind == '\0')) {
			if (new->data == NULL) {
				new->data = malloc(size);
				*data = new->data;
				return 1;
			}
			*data = new->data;
			return 0;
		}
		if (most_match != 0) {
			trie_element_t *old1 = new;
			trie_element_t *old2 = malloc(sizeof(trie_element_t));
			trie_element_t *new2 = NULL;
			char *tmp_ind = old1->ind;

			old2->ind = malloc((strlen(tmp_ind) - most_match + 1) * sizeof(char));
			strcpy(old2->ind, tmp_ind + most_match);
			old2->childs = old1->childs;
			old2->data = old1->data;

			if (strlen(ind) - most_match != 0) {
				new2 = malloc(sizeof(trie_element_t));
				new2->childs = NULL;
				new2->data = malloc(size);
				*data = new2->data;
				new2->ind = malloc((strlen(ind) - most_match + 1) * sizeof(char));
				strcpy(new2->ind, ind + most_match);
			}

			old1->ind = malloc((most_match + 1) * sizeof(char));
			strncpy(old1->ind, tmp_ind, most_match);
			old1->ind[most_match] = '\0';
			old1->childs = NULL;
			if (new2 == NULL) {
				old1->data = malloc(size);
				*data = old1->data;
			} else {
				old1->data = NULL;
			}
			old1->childs = g_slist_prepend(old1->childs, old2);
			if (new2 != NULL)
				old1->childs = g_slist_prepend(old1->childs, new2);

			free(tmp_ind);

			if (old1->ind == '\0' || old2->ind == '\0' || (new2 != NULL && new2->ind == '\0'))
				g_critical("trie created empty node");
			return 1;
		}
		cur = new;

	}
	return -1;
}

void* trie_get(trie_t* t, const char* ind)
{
	trie_element_t* cur = t->root;
	while (1) {
		cur = trie_element_match_child(cur->childs, &ind, NULL);
		if (unlikely(cur == NULL)) {
			return NULL;
		}
		if (unlikely(*ind == '\0')) {
			return cur->data;
		}
	}
	return NULL;
}

static void trie_iter_rec(trie_element_t *te, void itercall (void *data, void *user_data), void *user_data)
{
	GSList *child = te->childs;
	if (te->data != NULL)
		itercall(te->data, user_data);
	if (child == NULL)
		return;
	do {
		trie_iter_rec(child->data, itercall, user_data);
	} while (NULL != (child = g_slist_next(child)));
}

void trie_iter(trie_t *t, void itercall (void *data, void *user_data), void *user_data)
{
	trie_iter_rec(t->root, itercall, user_data);
}

