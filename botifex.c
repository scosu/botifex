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

#include "libirc.h"
#include "botifex_cmds.h"
#include "botifex_know.h"
#include "botifex.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>


static struct botifex boti;
static int shutdown = 0;


#define IRC

void *send_channel(void *user_data, struct bot_message *m);
void *send_user(void *user_data, struct bot_message *m);

void *irc_event_privmsg(irc_conn_t *c, struct irc_message *m)
{
	if (m->suffix[0] == '\001')
		return NULL;
	char buf[128];
	switch (bot_cmds_parse_msg(&boti, c, m)) {
	case -2:
		strcpy(buf, "ERROR not authorized");
		goto error_send;
	case -1:
		strcpy(buf, "ERROR command requires other parameters");
		goto error_send;
	case 0:
		strcpy(buf, "DONE");
error_send:
		if (m->middle[0] != '#') {
			struct bot_message bm = {
					.src = NULL,
					.dst = m->source,
					.msg = buf
			};
			send_user((void*) c, &bm);
		} else {
			struct bot_message bm = {
					.src = NULL,
					.dst = m->middle,
					.msg = buf
			};
			send_channel((void*) c, &bm);
		}
		return NULL;
	default:
		break;
	}
	bot_know_t *bot = boti.know;
	struct bot_message msg = {m->source, m->middle, m->suffix};
	if (m->middle[0] != '#') {
		bot_know_user_msg(bot, &msg, (void *) c);
		return NULL;
	} else {
		if (msg.msg[0] == '!') {
			++msg.msg;
			bot_know_channel_msg(bot, &msg, (void *) c, 1);
		} else {
			bot_know_channel_msg(bot, &msg, (void *) c, 0);
		}
	}

	return NULL;
}

void *send_channel(void *user_data, struct bot_message *m)
{
	if(user_data == NULL) {
		return NULL;
	}
#ifdef IRC
	irc_privmsg((irc_conn_t *) user_data, m->dst, m->msg);
#endif
	return NULL;
}

void *send_user(void *user_data, struct bot_message *m)
{
	irc_privmsg((irc_conn_t *) user_data, m->dst, m->msg);
	return NULL;
}

void sig_shutdown(int sig)
{
	shutdown = 1;
}

int main(int argc, char **argv)
{
	struct bot_callbacks bot_calls;
	bot_calls.send_channel = send_channel;
	bot_calls.send_user = send_user;
	boti.conns = NULL;
	boti.name = NULL;
	boti.know = bot_know_init(&bot_calls);
	boti.passwd = NULL;
	boti.authed = NULL;
	bot_know_set_talky(boti.know, 30);

	signal(SIGKILL, sig_shutdown);
	signal(SIGTERM, sig_shutdown);
	signal(SIGQUIT, sig_shutdown);

#ifndef IRC

	char buf[512];
	char buf2[512];
	bot_know_load(bot, "merastorum_buggy");
	bot_know_save(bot, "merastorum_buggy2");
	struct bot_message msg = {NULL, buf2, buf};
	strcpy(buf, "um was wollen wir wetten dass er jetzt kaputt geht?^^");
	strcpy(buf2, "#merastorum");
	bot_know_channel_msg(bot, &msg, NULL, 1);
	sleep(10);
	//bot_know_save(bot, "test");
	//bot_know_load(bot, "test");
	//bot_know_save(bot, "test2");

/*	strcpy(buf, "bli blub");
	bot_know_channel_msg(bot, &msg, NULL);*/

//	strcpy(buf, "blub blubba bla");
//	bot_know_channel_msg(bot, &msg, NULL);

#else
	GIOChannel *in = g_io_channel_unix_new(fileno(stdin));
	char *buf;
	gsize read_bytes;
	while (G_IO_STATUS_NORMAL == g_io_channel_read_line(in, &buf, &read_bytes, NULL, NULL) && !shutdown) {
		buf[read_bytes - 1] = '\0';
		if (buf[0] == '!') {
			struct irc_message msg = {
					.source = NULL,
					.cmd = NULL,
					.middle = NULL,
					.suffix = buf
			};
			bot_cmds_parse_msg(&boti, NULL, &msg);
		}
		g_free(buf);
	}
	while (!shutdown) {
		sleep(1);
	}
	GSList *cur = boti.conns;
	if (cur != NULL) {
		do {
			irc_disconnect(cur->data, "cya");
		} while ((cur = g_slist_next(cur)) != NULL);
	}
#endif
	return 0;
}

