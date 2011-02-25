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

#include <unistd.h>
#include <stdio.h>
#include <string.h>

void *irc_event_privmsg(irc_conn_t *c, struct irc_message *m)
{
	if (bot_cmds_parse(c, m) == 0)
		return NULL;
	bot_know_t *bot = (bot_know_t *) irc_get_user_data(c);
	struct bot_message msg = {m->source, m->middle, m->suffix};
	if (m->middle[0] != '#') {
		bot_know_user_msg(bot, &msg, (void *) c);
		return NULL;
	}
	bot_know_channel_msg(bot, &msg, (void *) c);
	return NULL;
}

void *send_channel(void *user_data, struct bot_message *m)
{
	irc_privmsg((irc_conn_t *) user_data, m->dst, m->msg);
	return NULL;
}

void *send_user(void *user_data, struct bot_message *m)
{
	irc_privmsg((irc_conn_t *) user_data, m->dst, m->msg);
	return NULL;
}

int main(int argc, char **argv)
{
	struct bot_callbacks bot_calls;
	bot_calls.send_channel = send_channel;
	bot_calls.send_user = send_user;
	bot_know_t *bot = bot_know_init(&bot_calls);

	struct irc_callbacks calls;
	memset(&calls, 0, sizeof(struct irc_callbacks));
	calls.privmsg = irc_event_privmsg;
	irc_conn_t *tmp = irc_connect("irc.euirc.net:6667", "botifex", "blub", NULL, &calls, bot);
	printf("waiting\n");
	sleep(5);
	irc_join_channel(tmp, "#merastorum");
	sleep(1);
	irc_privmsg(tmp, "#merastorum", "Testmessage");
	sleep(30);
	printf("exiting\n");
	irc_disconnect(tmp, "WAHAHAHA");
	sleep(2);
	printf("clean shutdown\n");
	return 0;
}

