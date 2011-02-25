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
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void *irc_event_privmsg(irc_conn_t *c, struct irc_message *m)
{
	if (m->middle[0] != '#')
		return NULL;
	printf("privmsg: %s\n", m->suffix);
	irc_privmsg(c, m->middle, m->suffix);
	return NULL;
}

int main(int argc, char **argv)
{
	struct irc_callbacks calls;
	memset(&calls, 0, sizeof(struct irc_callbacks));
	calls.privmsg = irc_event_privmsg;
	irc_conn_t *tmp = irc_connect("irc.euirc.net:6667", "scosu_lib_test", "blub", NULL, &calls);
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

