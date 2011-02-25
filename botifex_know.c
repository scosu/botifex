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

#include <glib.h>

struct bot_know_item {
	char *word;
	GSList next;
	GSList reply;
};

struct bot_relation {
	int weight;
	struct bot_know_item *dst;
};

bot_know_t *bot_know_init(struct bot_callbacks *callbacks)
{
	bot_know_t *b = malloc(sizeof(bot_know_t));
	if (b == NULL)
		return NULL;
	b->callbacks = *callbacks;
	b->words = trie_init();
	return b;
}

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

static int bot_know_get_first_seperator(const char *s)
{

	return -1;
}

static void bot_know_parse_sentence(bot_know_t *b, const char *to_parse,
		GSList *prev)
{

}

void bot_know_respond_to(bot_know_t *b, const char *msg, char *resp, size_t resp_size)
{
	strcpy(resp, "Don't get on my nerves!!!");
}

void bot_know_channel_msg(bot_know_t *b, struct bot_message *m, void *user_data)
{
	printf("chan_msg: %s\n", m->msg);
	struct bot_message tmp = {NULL, m->dst, m->msg};
	b->callbacks.send_channel(user_data, &tmp);
}

void bot_know_user_msg(bot_know_t *b, struct bot_message *m, void *user_data)
{
	printf("user_msg: %s\n", m->msg);
	struct bot_message tmp = {NULL, m->src, m->msg};
	b->callbacks.send_user(user_data, &tmp);
}
