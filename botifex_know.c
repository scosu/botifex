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

struct output_helper {
	GIOChannel *out;
	bot_know_t *b;
};

struct conversation {
	int last_action;
	int next_action;
	int say_something;
	char *identifier;
	void *user_data;
	GSList *last_sent;
};

static struct bot_know_item *bot_know_get_word(bot_know_t *b, char *word, int length);
static void bot_know_relation_inc(GSequence *rel_cont, struct bot_know_item *dst,
		int inc);
static void bot_know_reply(bot_know_t *b, struct conversation *conv, int reply);
void *bot_know_sayer(void *data);


void bot_know_dump_assocs(GSequence *s, struct output_helper *h) {
	char buf[128];
	GSequenceIter *j = g_sequence_get_begin_iter(s);
	if (j != NULL) {
		while (!g_sequence_iter_is_end(j)) {
			struct bot_know_relation *rel = g_sequence_get(j);
			sprintf(buf, " %d ", rel->weight);
			g_io_channel_write_chars(h->out, buf, -1, NULL, NULL);
			if (rel->dst != &h->b->end)
				g_io_channel_write_chars(h->out, rel->dst->word, -1, NULL, NULL);
			g_io_channel_write_chars(h->out, "\n", -1, NULL, NULL);
			j = g_sequence_iter_next(j);
		}
	}
}

void bot_know_dump_item(struct bot_know_item *i, struct output_helper *h)
{
	if (i->word[0] == '\0')
		g_critical("Dumping an empty item");
	g_io_channel_write_chars(h->out, i->word, -1, NULL, NULL);
	g_io_channel_write_chars(h->out, "\n", -1, NULL, NULL);
	bot_know_dump_assocs(i->nexts, h);
	g_io_channel_write_chars(h->out, " \n", -1, NULL, NULL);
	bot_know_dump_assocs(i->assoc, h);
}

int bot_know_save(bot_know_t *b, const char *path)
{
	struct output_helper h;
	h.out = g_io_channel_new_file(path, "w", NULL);
	h.b = b;
	bot_know_dump_item(&b->start, &h);
	trie_iter((void *) b->words, (void *) bot_know_dump_item, &h);
	g_io_channel_shutdown(h.out, 1, NULL);
	return 0;
}

int bot_know_load(bot_know_t *b, const char *path)
{
	GIOChannel *in = g_io_channel_new_file(path, "r", NULL);
	char *buf;
	enum {
		PARSER_SENT_REL,
		PARSER_ASSOC_REL
	} parser_state = PARSER_SENT_REL;
	struct bot_know_item *citem = &b->start;
	int first = 1;
	gsize read;
	while (G_IO_STATUS_NORMAL == g_io_channel_read_line(in, &buf, &read, NULL, NULL)) {
		buf[read - 1] = '\0';
		if (buf[0] == ' ') {
			if (buf[1] == '\0') {
				++parser_state;
			} else {
				int i;
				for (i = 1; buf[i] != ' '; ++i) {}
				buf[i] = '\0';
				int weight = g_ascii_strtoll(buf + 1, NULL, 0);
				struct bot_know_item *dst;
				if (buf[i + 1] == '\0')
					dst = &b->end;
				else
					dst = bot_know_get_word(b, buf + i + 1, -1);

				switch (parser_state) {
				case PARSER_SENT_REL:
					bot_know_relation_inc(citem->nexts, dst, weight);
					break;
				case PARSER_ASSOC_REL:
					bot_know_relation_inc(citem->assoc, dst, weight);
					break;
				}
			}
		} else if (buf[0] != '\0'){
			parser_state = PARSER_SENT_REL;
			if (first)
				first = 0;
			else
				citem = bot_know_get_word(b, buf, -1);
		}
		g_free(buf);
	}
	return 0;
}

void bot_know_set_file(bot_know_t *b, const char *path)
{
	if (b->path != NULL) {
		free(b->path);
	}
	b->path = malloc((strlen(path) + 1) * sizeof(char));
	strcpy(b->path, path);
}

int bot_know_reset(bot_know_t *b)
{
	return 0;
}

#define DUMP_INTERVAL 360

void *bot_know_run(void *data)
{
	bot_know_t *b = data;
	int dump_know = 0;
	while (!b->shutdown) {
		g_usleep(500000);
		if (b->path && ++dump_know == DUMP_INTERVAL) {
			dump_know = 0;
			g_static_mutex_lock(&b->lock);
			if (b->path) {
				bot_know_save(b, b->path);
			}
			g_static_mutex_unlock(&b->lock);
			g_static_mutex_unlock(&b->say_something);
		}
		if (b->talky) {
			GSequenceIter *i = g_sequence_get_begin_iter(b->conversations);
			while (!g_sequence_iter_is_end(i)) {
				struct conversation *conv = g_sequence_get(i);
				if (++(conv->last_action) == conv->next_action) {
					conv->say_something = 1;
					g_static_mutex_unlock(&b->say_something);
					conv->last_action = 0;
					conv->next_action = g_random_int_range(200, 500 + (100 - b->talky) * 30);
				}
				i = g_sequence_iter_next(i);
			}
		}
	}
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
	b->shutdown = 0;
	b->path = NULL;
	bot_know_item_init(&b->start, "start");
	bot_know_item_init(&b->end, "end");
	b->conversations = g_sequence_new(NULL);
	g_thread_init(NULL);
	g_static_mutex_init(&b->lock);
	g_static_mutex_unlock(&b->lock);
	g_static_mutex_init(&b->say_something);
	g_static_mutex_trylock(&b->say_something);
	b->manager = g_thread_create(bot_know_run, b, 1, NULL);
	b->sayer = g_thread_create(bot_know_sayer, b, 1, NULL);
	return b;
}

static struct bot_know_item *bot_know_get_word(bot_know_t *b, char *word, int length)
{
	struct bot_know_item *item;
	char replaced;
	if (length != -1) {
		replaced = word[length];
		word[length] = '\0';
	}
	int created = trie_get_or_create(b->words, (void **)&item, word,
			sizeof(struct bot_know_item));
	if (created) {
		bot_know_item_init(item, word);
		++b->wordct;
	}
	if (length != -1)
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
		GSList *prev)
{
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
									((struct bot_know_item *) j->data), 80);
							bot_know_relation_inc(
									((struct bot_know_item *) j->data)->assoc,
									((struct bot_know_item *) i->data), 30);
						} while (NULL != (j = g_slist_next(j)));
					} while (NULL != (i = g_slist_next(i)));
				}
			}
		}
	}
	sent = g_slist_reverse(sent);
	return sent;
}



static void bot_know_parse_msg(bot_know_t *b, char *to_parse,
		struct conversation *conv, int force_reply)
{
	conv->last_action = 0;
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
				GSList *sent_tmp = conv->last_sent;
				conv->last_sent = bot_know_parse_sentence(b, to_parse, conv->last_sent);
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
		GSList *sent_tmp = conv->last_sent;
		conv->last_sent = bot_know_parse_sentence(b, to_parse, conv->last_sent);
		g_slist_free(sent_tmp);
		to_parse = to_parse + i;
		to_parse[0] = tmp;
		i = 0;
	}

	if (force_reply || (b->talky && g_random_int_range(0, 100) <= b->talky)) {
		conv->say_something = 2;
		g_static_mutex_unlock(&b->say_something);
	}
}

static void bot_know_reply(bot_know_t *b, struct conversation *conv, int reply)
{
	char buf[512] = "";
	char *bufc = buf;

	struct bot_know_item *itemc = &b->start;
	if (conv->last_sent == NULL)
		reply = 0;

	while (1) {
		long sum = 0;
		if (reply) {
			GSList *clast = conv->last_sent;
			while (clast != NULL) {
				GSequenceIter *e = g_sequence_get_begin_iter(((struct bot_know_item *) clast->data)->assoc);
				while (!g_sequence_iter_is_end(e)) {
					GSequenceIter *tmp = g_sequence_lookup(itemc->nexts, g_sequence_get(e), bot_know_item_cmp, NULL);
					if (tmp != NULL) {
						struct bot_know_relation *last_rel = g_sequence_get(tmp);
						sum += last_rel->weight;
					}
					e = g_sequence_iter_next(e);
				}
				clast = g_slist_next(clast);
			}
		}
		GSequenceIter *i = g_sequence_get_begin_iter(itemc->nexts);
		struct bot_know_relation *relc = g_sequence_get(i);
		int rand = g_random_int_range(0, g_sequence_get_length(itemc->nexts) + sum + 1);
		while (rand > 0 && !g_sequence_iter_is_end(i)) {
			if (reply) {
				GSList *clast = conv->last_sent;
				while (clast != NULL) {
					struct bot_know_item *lastitem = clast->data;
					GSequenceIter *tmp = g_sequence_lookup(lastitem->assoc, g_sequence_get(i), bot_know_item_cmp, NULL);
					if (tmp != NULL) {
						struct bot_know_relation *last_rel = g_sequence_get(tmp);
						rand -= last_rel->weight;
					}
					clast = g_slist_next(clast);
				}
			}
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
			buf[511] = '\0';
			break;
		}
		strcpy(bufc, " ");
		++bufc;
	}
	--bufc;
	*bufc = '\0';
	if (buf[0] == '\0')
		return;
	struct bot_message tmp = {NULL, conv->identifier, buf};
	g_usleep(150000 * (bufc - buf));
	b->callbacks.send_channel(conv->user_data, &tmp);
}

gint bot_know_conv_cmp(gconstpointer a, gconstpointer b, gpointer user_data)
{
	const struct conversation *ac = a;
	const struct conversation *bc = b;
	return strcmp(ac->identifier, bc->identifier);
}

static struct conversation *bot_know_get_create_conversation(
		bot_know_t *b, char *identifier, void *user_data)
{
	struct conversation search;
	search.identifier = identifier;
	GSequenceIter *i = g_sequence_lookup(b->conversations, &search,
			bot_know_conv_cmp, NULL);
	if (i == NULL) {
		struct conversation *tmp = malloc(sizeof(struct conversation));
		tmp->identifier = malloc((strlen(identifier) + 1) * sizeof(char));
		strcpy(tmp->identifier, identifier);
		tmp->last_action = 0;
		tmp->last_sent = NULL;
		tmp->say_something = 0;
		tmp->next_action = g_random_int_range(1000, 1000 + (100 - b->talky) * 10);
		tmp->user_data = user_data;
		i = g_sequence_insert_sorted(b->conversations, tmp, bot_know_conv_cmp, NULL);
	}
	return g_sequence_get(i);
}

void bot_know_channel_msg(bot_know_t *b, struct bot_message *m, void *user_data,
		int force_reply)
{
	struct conversation *conv = bot_know_get_create_conversation(b, m->dst, user_data);
	bot_know_parse_msg(b, m->msg, conv, force_reply);
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

void *bot_know_sayer(void *data)
{
	bot_know_t *b = data;
	while (!b->shutdown) {
		g_static_mutex_lock(&b->say_something);
		GSequenceIter *i = g_sequence_get_begin_iter(b->conversations);
		while (!g_sequence_iter_is_end(i)) {
			struct conversation *conv = g_sequence_get(i);
			if (conv->say_something) {
				bot_know_reply(b, conv, conv->say_something - 1);
				conv->say_something = 0;
			}
			i = g_sequence_iter_next(i);
		}
	}
	return NULL;
}
