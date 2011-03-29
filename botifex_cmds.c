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
#include "botifex.h"

#include <string.h>
#include <stdio.h>

struct irc_callbacks irc_calls = {
		.privmsg = irc_event_privmsg
};

enum bot_cmds {
	CMD_LEAVE = 0,
	CMD_JOIN,
	CMD_CONNECT,
	CMD_DISCONNECT,
	CMD_SAVE,
	CMD_LOAD,
	CMD_RESET,
	CMD_LEARN,
	CMD_TALKY,
	CMD_AUTH,
	CMD_DEAUTH,
	CMD_PASSWD,
	CMD_PERM,
	CMD_NICK,
	CMD_AUTOSAVE,
	CMD_NONE
};

static const char *(cmds[CMD_NONE]) = {
		"leave",
		"join",
		"connect",
		"disconnect",
		"save",
		"load",
		"reset",
		"learn",
		"talky",
		"auth",
		"deauth",
		"passwd",
		"perm",
		"nick",
		"autosave"
};

static int right_needed[CMD_NONE] = {
		0,
		1,
		1,
		1,
		1,
		1,
		1,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		1
};

int bot_cmds_parse(const char *c)
{
	int i = CMD_NONE;
	for (; i--;) {
		printf("comparing: '%s' | '%s'\n", cmds[i], c);
		if (g_str_has_prefix(c, cmds[i]))
			return i;
	}
	return CMD_NONE;
}

int bot_cmds_parse_msg(struct botifex *bot, irc_conn_t *c, struct irc_message *m)
{
	if (m->suffix[0] != '!') {
		return 1;
	}
	++m->suffix;
	enum bot_cmds icode = bot_cmds_parse(m->suffix);
	printf("code: %d\n", icode);
	int cmd_len = strlen(cmds[icode]);
	printf("cmd_len: %d\n", cmd_len);
	if (icode == CMD_NONE || (m->suffix[cmd_len] != '\0' && m->suffix[cmd_len] != ' '))
		return 1;
	if (right_needed[icode] && c != NULL && g_strcmp0(m->source, bot->authed) != 0)
		return -2;
	char *params;
	if (m->suffix[cmd_len] == ' ') {
		params = m->suffix + cmd_len + 1;
		printf("params: %s\n", params);
	} else
		params = NULL;
	if (c == NULL) {
		if (bot->conns != NULL)
			c = bot->conns->data;
	}
	switch (icode) {
	case CMD_LEAVE:
		if (params == NULL)
			return -1;
		irc_leave_channel(c, params);
		break;
	case CMD_JOIN:
		if (params == NULL)
			return -1;
		irc_join_channel(c, params);
		break;
	case CMD_CONNECT:
		if (params == NULL)
			return -1;
		bot->conns = g_slist_prepend(bot->conns, irc_connect(params, bot->name, "botifex", NULL, &irc_calls, bot));
		break;
	case CMD_DISCONNECT:
		if (params == NULL) {
			bot->conns = g_slist_remove(bot->conns, c);
			irc_disconnect(c, "cya");
		} else {
			return -1;
		}
		break;
	case CMD_LEARN: {
		if (params == NULL)
			return -1;
		int learn = atoi(params);
		bot_know_set_learn(bot->know, learn);
		break;
	}
	case CMD_TALKY: {
		if (params == NULL)
			return -1;
		int talky = atoi(params);
		if (talky < 0 || talky > 100)
			return -1;
		bot_know_set_talky(bot->know, talky);
		break;
	}
	case CMD_AUTOSAVE:
		bot_know_set_file(bot->know, params);
		break;
	case CMD_LOAD:
		if (params == NULL)
			return -1;
		bot_know_load(bot->know, params);
		break;
	case CMD_RESET:
		bot_know_reset(bot->know);
		break;
	case CMD_AUTH:
		if (params == NULL)
			return -1;
		if (g_strcmp0(params, bot->passwd) == 0) {
			if (bot->authed != NULL)
				free(bot->authed);
			bot->authed = malloc(strlen(m->source) + 1);
			strcpy(bot->authed, m->source);
		} else {
			return -2;
		}
		break;
	case CMD_DEAUTH:
		free(bot->authed);
		bot->authed = NULL;
		break;
	case CMD_PASSWD:
		if (params == NULL)
			return -1;
		if (bot->passwd != NULL)
			free(bot->passwd);
		bot->passwd = malloc(strlen(params) + 1);
		strcpy(bot->passwd, params);
		break;
	case CMD_PERM: {
		if (params == NULL)
			return -1;
		int code = bot_cmds_parse(params);
		if (code == CMD_NONE)
			return -1;
		int enable = atoi(params + strlen(cmds[code]) + 1);
		right_needed[code] = enable;
		break;
	}
	case CMD_NICK:
		if (params == NULL)
			return -1;
		if (bot->name != NULL)
			free(bot->name);

		bot->name = malloc(strlen(params) + 1);
		strcpy(bot->name, params);
		if (c != NULL)
			irc_set_nick(c, params);
		break;
	default:
		break;
	}
	return 0;
}
