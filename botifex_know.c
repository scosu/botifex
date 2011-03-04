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

#include "botifex_know.h"
#include "trie.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

struct bot_know_relation {
	int weight;
	struct bot_know_item *dst;
};



int bot_know_store(bot_know_t *b, const char *path)
{
	return 0;
}

int bot_know_load(bot_know_t *b, const char *path)
{
	return 0;
}

int bot_know_reset(bot_know_t *b)
{
	return 0;
}

void *bot_know_run(void *data)
{
	bot_know_t *b = data;
	return NULL;
}


static const char *sentence_seperators = ".?!";
static const char *word_seperators = " ;:-+#\"*~/',^";

#define SEP_TYPE_BOTH 0
#define SEP_TYPE_SENT 1
#define SEP_TYPE_WORD 2

static int bot_know_get_first_seperator(const char *s, int sep_type)
{
	int i;
	for (i = 0; s[i] != '\0'; ++i) {
		int j;
		if (sep_type == SEP_TYPE_BOTH || sep_type == SEP_TYPE_SENT) {
			for (j = 0; sentence_seperators[j] != '\0'; ++j) {
				if (sentence_seperators[j] == s[i])
					return i;
			}
		}
		if (sep_type == SEP_TYPE_BOTH || sep_type == SEP_TYPE_WORD) {
			for (j = 0; word_seperators[j] != '\0'; ++j) {
				if (word_seperators[j] == s[i])
					return i;
			}
		}
	}
	return -1;
}

static void bot_know_item_init(struct bot_know_item *item, const char *word)
{
	item->word = malloc(strlen(word) + 1);
	strcpy(item->word, word);
	item->nexts = g_sequence_new(NULL);
	item->assoc = g_sequence_new(NULL);
}

bot_know_t *bot_know_init(struct bot_callbacks *callbacks)
{
	bot_know_t *b = malloc(sizeof(bot_know_t));
	if (b == NULL)
		return NULL;
	b->callbacks = *callbacks;
	b->words = trie_init();
	b->talky = 0;
	b->learn = 1;
	bot_know_item_init(&b->start, "start");
	bot_know_item_init(&b->end, "end");
	return b;
}

static struct bot_know_item *bot_know_get_word(bot_know_t *b, char *word, int length)
{
	struct bot_know_item *item;
	const char replaced = word[length];
	word[length] = '\0';
	int created = trie_get_or_create(b->words, (void **)&item, word,
			sizeof(struct bot_know_item));
	if (created) {
		bot_know_item_init(item, word);
		++b->wordct;
	} else {
		printf("found a word: %s\n", word);
	}
	word[length] = replaced;
	return item;
}

static struct bot_know_relation *bot_know_relation_init(struct bot_know_item *dst)
{
	struct bot_know_relation *rel = malloc(sizeof(struct bot_know_relation));
	rel->dst = dst;
	rel->weight = 0;
	return rel;
}

gint bot_know_item_cmp(gconstpointer a, gconstpointer b, gpointer user_data)
{
	if (a < b)
		return -1;
	if (a == b)
		return 0;
	return 1;
}

static void bot_know_relation_inc(GSequence *rel_cont, struct bot_know_item *dst,
		int inc)
{
	GSequenceIter *found = g_sequence_lookup(rel_cont, dst,
			bot_know_item_cmp, NULL);
	struct bot_know_relation* rel;
	if (found == NULL) {
		rel = bot_know_relation_init(dst);
		found = g_sequence_insert_sorted(rel_cont, rel, bot_know_item_cmp,
				NULL);
	} else {
		rel = g_sequence_get(found);
	}
	rel->weight += inc;
	g_sequence_sort_changed(found, bot_know_item_cmp, NULL);
}

static void bot_know_learn_word(bot_know_t *b, struct bot_know_item *sprev,
		GSList *prev, struct bot_know_item *word)
{
	printf("rel_inc: %s -> %s\n", sprev->word, word->word);
	bot_know_relation_inc(sprev->nexts, word, 1);
	GSList *i = prev;
	if (i != NULL) {
		do {
			bot_know_relation_inc(
					((struct bot_know_item *) i->data)->assoc, word, 1);
			bot_know_relation_inc(
					word->assoc, ((struct bot_know_item *) i->data), 1);
		} while (NULL != (i = g_slist_next(i)));
	}
}

static GSList *bot_know_parse_sentence(bot_know_t *b, char *to_parse,
		GSList *prev, const char *message_dst, void *user_data)
{
	int sep;
	struct bot_know_item *litem = &b->start;
	GSList *sent = NULL;
	if (b->learn) {
		enum {
			PARSE_NOTHING,
			PARSE_WORD,
			PARSE_SEPERATOR,
			PARSE_SEPERATOR_WORD
		} parse_state = PARSE_NOTHING;
		while (1) {
			int i = 0;
			int sep;
			int parse_word = 0;
			switch (parse_state) {
			case PARSE_NOTHING:
				while (to_parse[i] == ' ') {
					++i;
				}
				to_parse += i;
				sep = bot_know_get_first_seperator(to_parse, SEP_TYPE_BOTH);
				if (sep == 0)
					parse_state = PARSE_SEPERATOR_WORD;
				else
					parse_state = PARSE_WORD;
				break;
			case PARSE_WORD:
				sep = bot_know_get_first_seperator(to_parse, SEP_TYPE_BOTH);
				if (sep != -1) {
					if (to_parse[sep] == ' ') {
						parse_word = 1;
						break;
					}
					i = sep;
					int res;
					do {
						++i;
						res = bot_know_get_first_seperator(to_parse + i, SEP_TYPE_BOTH);
					} while (res == 0 && to_parse[i] != ' ');
					if (to_parse[i] == ' ' || to_parse[i] == '\0') {
						parse_word = 1;
						parse_state = PARSE_SEPERATOR_WORD;
						break;
					} else {
						parse_state = PARSE_SEPERATOR_WORD;
						break;
					}
				} else {
					sep = strlen(to_parse);
				}
				parse_word = 1;
				break;
			case PARSE_SEPERATOR:
				break;
			case PARSE_SEPERATOR_WORD:
				for (sep = 0; to_parse[sep] != ' ' && to_parse[sep] != '\0'; ++sep) {}
				parse_word = 1;
				break;
			}

			if (parse_word) {
				struct bot_know_item *citem = bot_know_get_word(b, to_parse, sep);
				sent = g_slist_prepend(sent, citem);
				bot_know_learn_word(b, litem, prev, citem);
				litem = &(*citem);
				to_parse += sep;
				parse_state = PARSE_NOTHING;
			}
			if (to_parse[0] == '\0')
				break;
		}
		if (litem != &b->start) {
			bot_know_learn_word(b, litem, prev, &b->end);
			{
				GSList *i = sent;
				if (i != NULL) {
					do {
						GSList *j = sent;
						if (j == NULL) {
							break;
						}
						do {
							if (j->data == i->data)
								continue;

							bot_know_relation_inc(
									((struct bot_know_item *) i->data)->assoc,
									((struct bot_know_item *) j->data), 1);
							bot_know_relation_inc(
									((struct bot_know_item *) j->data)->assoc,
									((struct bot_know_item *) i->data), 1);
						} while (NULL != (j = g_slist_next(j)));
					} while (NULL != (i = g_slist_next(i)));
				}
			}
		}
	}
	sent = g_slist_reverse(sent);
	return sent;
}



static GSList *bot_know_parse_msg(bot_know_t *b, char *to_parse,
		GSList *last_sent, const char *message_dst, void *user_data)
{
/*	int i;
	enum {
		SEPERATORS_0,
		SEPERATORS_1,
		SEPERATORS_IGNORE
	} state = SEPERATORS_0;*/

	while (1) {
		int sep = 0;
		int i = 0;
		int seps;
search_again:
		seps = 0;
		sep = bot_know_get_first_seperator(to_parse + i, SEP_TYPE_SENT);
		if (sep == -1) {
			goto out;
		}
		i += sep - 1;
		do {
			++i;
			sep = bot_know_get_first_seperator(to_parse + i, SEP_TYPE_BOTH);
			++seps;
		} while (sep == 0 && to_parse[i + 1] != ' ');
		if (sep == -1) {
out:
			if (to_parse[0] != '\0') {
				GSList *sent_tmp = last_sent;
				last_sent = bot_know_parse_sentence(b, to_parse, last_sent, message_dst, user_data);
				g_slist_free(sent_tmp);
			}
			break;
		}
		i += sep;
		if (seps != 1 || to_parse[i] == ' ') {
			goto search_again;
		}
		++i;
		char tmp = to_parse[i];
		to_parse[i] = '\0';
		GSList *sent_tmp = last_sent;
		last_sent = bot_know_parse_sentence(b, to_parse, last_sent, message_dst, user_data);
		g_slist_free(sent_tmp);
		to_parse = to_parse + i;
		to_parse[0] = tmp;
		i = 0;
	}

	if (b->talky) {
		char buf[512] = "";
		char *bufc = buf;

		struct bot_know_item *itemc = &b->start;

		while (1) {
			GSequenceIter *i = g_sequence_get_begin_iter(itemc->nexts);
			struct bot_know_relation *relc = g_sequence_get(i);
			int poss = g_sequence_get_length(itemc->nexts);
			int rand = g_random_int_range(0, g_sequence_get_length(itemc->nexts) + 1);
			while (rand > 0 && !g_sequence_iter_is_end(i)) {
				if (rand > 1)
					printf("yes\n");
				relc = g_sequence_get(i);
				rand -= relc->weight;
				i = g_sequence_iter_next(i);
			}
			itemc = relc->dst;
			if (itemc == &b->end)
				break;
			if (0 == bot_know_get_first_seperator(itemc->word, SEP_TYPE_BOTH)
					&& buf != bufc)
				--bufc;
			strncpy(bufc, itemc->word, buf + 512 - bufc);
			bufc += strlen(itemc->word);
			if (bufc - buf + 2 >= 512) {
				buf[512] = '\0';
				break;
			}
			strcpy(bufc, " ");
			++bufc;
		}
		struct bot_message tmp = {NULL, message_dst, buf};
		b->callbacks.send_channel(user_data, &tmp);
	}
	return last_sent;
}

void bot_know_channel_msg(bot_know_t *b, struct bot_message *m, void *user_data)
{
	GSList *last_sent = bot_know_parse_msg(b, m->msg, NULL, m->dst, user_data);
	g_slist_free(last_sent);
}

void bot_know_user_msg(bot_know_t *b, struct bot_message *m, void *user_data)
{
	//bot_know_channel_msg(b, m, user_data);
}

void bot_know_set_learn(bot_know_t *b, int enable)
{
	b->learn = enable;
}

void bot_know_set_talky(bot_know_t *b, int level)
{
	b->talky = level;
}
