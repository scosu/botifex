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

#ifndef BOTIFEX_KNOW_H_
#define BOTIFEX_KNOW_H_

#define BOTIFEX_KNOW_VERSION 0.1.0

#include "trie.h"

struct bot_message {
	char *src;
	char *dst;
	char *msg;
};

struct bot_know_item {
	char *word;
	GSequence *nexts;
	GSequence *assoc;
};

struct bot_callbacks {
	void (*send_channel)(void *, struct bot_message *);
	void (*send_user)(void *, struct bot_message *);
};

typedef struct {
	struct bot_callbacks callbacks;
	int wordct;
	trie_t *words;

	struct bot_know_item start;
	struct bot_know_item end;

	int learn;
	int talky;

	int shutdown;

	char *path;

	GThread *thread;
	GStaticMutex lock;

	GSequence *conversations;
} bot_know_t;


bot_know_t *bot_know_init(struct bot_callbacks *callbacks);
void bot_know_set_file(bot_know_t *b, const char *path);
int bot_know_save(bot_know_t *b, const char *path);
int bot_know_load(bot_know_t *b, const char *path);
int bot_know_reset(bot_know_t *b);
void bot_know_channel_msg(bot_know_t *b, struct bot_message *m, void *user_data,
		int force_reply);
void bot_know_user_msg(bot_know_t *b, struct bot_message *m, void *user_data);
void bot_know_set_learn(bot_know_t *b, int enable);
void bot_know_set_talky(bot_know_t *b, int level);

#endif /* BOTIFEX_KNOW_H_ */
