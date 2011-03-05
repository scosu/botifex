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

#include <glib.h>
#include <gio/gio.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libirc.h"


enum {
	IRC_A = IRC_ADMIN,
	IRC_C = IRC_CONNECT,
	IRC_E = IRC_ERROR,
	IRC_I = IRC_INFO,
	IRC_J = IRC_JOIN,
	IRC_K = IRC_KICK,
	IRC_L = IRC_LINKS,
	IRC_M = IRC_MODE,
	IRC_N = IRC_NAMES,
	IRC_O = IRC_OPER,
	IRC_P = IRC_PART,
	IRC_Q = IRC_QUIT,
	IRC_R = IRC_REHASH,
	IRC_S = IRC_SERVER,
	IRC_T = IRC_TIME,
	IRC_U = IRC_USER,
	IRC_V = IRC_VERSION,
	IRC_W = IRC_WALLOPS
};

static const char *(cmds[IRC_NUM_CMDS]) = {
		"ADMIN",
		"AWAY",
		"CONNECT",
		"ERROR",
		"INFO",
		"INVITE",
		"ISON",
		"JOIN",
		"KICK",
		"KILL",
		"LINKS",
		"LIST",
		"MODE",
		"NAMES",
		"NICK",
		"NOTICE",
		"OPER",
		"PART",
		"PASS",
		"PING",
		"PONG",
		"PRIVMSG",
		"QUIT",
		"REHASH",
		"RESTART",
		"SERVER",
		"SQUIT",
		"STATS",
		"SUMMON",
		"TIME",
		"TOPIC",
		"TRACE",
		"USER",
		"USERHOST",
		"USERS",
		"VERSION",
		"WALLOPS",
		"WHO",
		"WHOIS",
		"WHOWAS"
};

static void irc_cmd_msg(irc_conn_t *c, int cmd, struct irc_message *m)
{
	char buf[512] = "";
	if (m->source != NULL) {
		strcat(buf, ":");
		strcat(buf, m->source);
		strcat(buf, " ");
	}
	strcat(buf, cmds[cmd]);
	if (m->middle != NULL) {
		strcat(buf, " ");
		strcat(buf, m->middle);
	}
	if (m->suffix != NULL) {
		strcat(buf, " :");
		strcat(buf, m->suffix);
	}
	gsize written;
	strcat(buf, "\n");
	g_output_stream_write_all(c->out, buf, strlen(buf), &written, NULL, NULL);
}

static void irc_cmd(irc_conn_t *c, int cmd, const char *middle, const char *suffix)
{
	struct irc_message m = {NULL, NULL, middle, suffix};
	irc_cmd_msg(c, cmd, &m);
}

static void irc_parse_subcmd(irc_conn_t *c, char *cmd, struct irc_message *m, int substart)
{
	void (**call)(irc_conn_t *, struct irc_message *);
	while (substart < IRC_NUM_CMDS && cmds[substart][0] == cmd[0]) {
		int i;
		for (i = 1; cmds[substart][i] != '\0' && cmd[i] != ' '; ++i) {
			if (cmds[substart][i] != cmd[i])
				break;
		}
		if (cmds[substart][i] == '\0' && cmd[i] == ' ') {
			cmd[i] = '\0';
			cmd += i + 1;
			if (cmd[0] != ':') {
				m->middle = cmd;
				int j;
				for (j = 0; cmd[j] != '\0'
						&& (cmd[j] != ' ' || cmd[j + 1] != ':'); ++j) {}
				if (cmd[j] != '\0' && cmd[j] == ' ' && cmd[j + 1] == ':') {
					cmd[j] = '\0';
					m->suffix = cmd + j + 2;
				} else {
					m->suffix = NULL;
				}
			}
			else
			{
				m->suffix = cmd + 1;
				m->middle = NULL;
			}
			call = (void*)&c->callbacks;
			call += substart;
			if (*call != NULL)
				(**call)(c, m);
			return;
		}
		++substart;
	}
}

static void irc_parse_cmd(irc_conn_t *c, char *cmd)
{
	struct irc_message m;
	int start_compare = 0;
	if (cmd[0] == ':') {
		m.source = cmd + 1;
		int i;
		for (i = 1; cmd[i] != '\0' && cmd[i] != ' '; ++i) {
			if (cmd[i] == '!')
				cmd[i] = '\0';
		}
		if (cmd[i] == '\0')
			return;
		cmd[i] = '\0';
		cmd = cmd + i + 1;
	} else {
		m.source = NULL;
	}
	m.cmd = cmd;
	switch(cmd[0]) {
	case 'A':
		start_compare = IRC_A;
		break;
	case 'C':
		start_compare = IRC_C;
		break;
	case 'E':
		start_compare = IRC_E;
		break;
	case 'I':
		start_compare = IRC_I;
		break;
	case 'J':
		start_compare = IRC_J;
		break;
	case 'K':
		start_compare = IRC_K;
		break;
	case 'L':
		start_compare = IRC_L;
		break;
	case 'M':
		start_compare = IRC_M;
		break;
	case 'N':
		start_compare = IRC_N;
		break;
	case 'O':
		start_compare = IRC_O;
		break;
	case 'P':
		start_compare = IRC_P;
		break;
	case 'Q':
		start_compare = IRC_Q;
		break;
	case 'R':
		start_compare = IRC_R;
		break;
	case 'S':
		start_compare = IRC_S;
		break;
	case 'T':
		start_compare = IRC_T;
		break;
	case 'U':
		start_compare = IRC_U;
		break;
	case 'V':
		start_compare = IRC_V;
		break;
	case 'W':
		start_compare = IRC_W;
		break;
	default:
		printf("no CMD: %s\n", cmd);
		return;
	}
	irc_parse_subcmd(c, cmd, &m, start_compare);
}

static void *irc_listener(void *data)
{
	irc_conn_t *c = (irc_conn_t*) data;
	char *buf;
	gsize readct;
	printf("start reading\n");
	GDataInputStream *input = g_data_input_stream_new(c->in);
	while(NULL != (buf = g_data_input_stream_read_line(input, &readct, NULL, NULL))
			&& !c->shutdown) {
		printf("[debug] %s\n", buf);
		irc_parse_cmd(c, buf);
		g_free(buf);
	}
	printf("end reading\n");
	g_object_unref(c->in);
	free(c);
	printf("cleaned struct\n");
	return NULL;
}

static void *irc_ping(irc_conn_t *c, struct irc_message *m)
{
	irc_cmd(c, IRC_PONG, NULL, m->suffix);
	return NULL;
}

irc_conn_t *irc_connect(const char *host_port, const char *nick, const char *name,
		const char *passwd, struct irc_callbacks *callbacks, void *data)
{
	g_type_init();
	if (!g_thread_get_initialized())
		g_thread_init(NULL);
	irc_conn_t *res = malloc(sizeof(irc_conn_t));
	if (res == NULL)
		return NULL;

	res->sock = g_socket_client_new();
	res->conn = g_socket_client_connect_to_host(res->sock, host_port, 0, NULL, NULL);
	if (res->conn == NULL) {
		g_object_unref(res->sock);
		free(res);
		return NULL;
	}
	res->in = g_io_stream_get_input_stream((GIOStream*) res->conn);
	res->out = g_io_stream_get_output_stream((GIOStream*) res->conn);
	res->user_data = data;

	memset(&res->callbacks, 0, sizeof(struct irc_callbacks));
	res->callbacks.ping = irc_ping;
	if (callbacks != NULL) {
		void (**callres)(irc_conn_t *, struct irc_message *) = &res->callbacks.whowas + 1;
		void (**calluser)(irc_conn_t *, struct irc_message *) = &callbacks->whowas + 1;
		while ((void *) callres != (void *) &res->callbacks) {
			--callres;
			--calluser;
			if (*calluser != NULL)
				*callres = *calluser;
		}
	}


	res->thread = g_thread_create(irc_listener, res, FALSE, NULL);
	char buf[512];
	if (passwd != NULL)
		irc_cmd(res, IRC_PASS, passwd, NULL);
	irc_cmd(res, IRC_NICK, nick, NULL);

	sprintf(buf, "%s %s %s %s", nick, nick, name, name);
	irc_cmd(res, IRC_USER, buf, NULL);

	printf("done connecting\n");
	return res;
}

void irc_set_nick(irc_conn_t *c, const char *nick)
{
	irc_cmd(c, IRC_NICK, nick, NULL);
}

void irc_join_channel(irc_conn_t *c, const char *channel)
{
	irc_cmd(c, IRC_JOIN, channel, NULL);
}

void irc_leave_channel(irc_conn_t *c, const char *channel)
{
	irc_cmd(c, IRC_PART, channel, NULL);
}

void irc_privmsg(irc_conn_t *c, const char *dst, const char *msg)
{
	irc_cmd(c, IRC_PRIVMSG, dst, msg);
}

void irc_disconnect(irc_conn_t *c, const char *msg)
{
	irc_cmd(c, IRC_QUIT, NULL, msg);
	GOutputStream *tmp_out = c->out;
	GSocketConnection *tmp_conn = c->conn;
	GSocketClient *tmp_sock = c->sock;
	c->shutdown = 1;
	g_object_unref(tmp_out);
	g_object_unref(tmp_conn);
	g_object_unref(tmp_sock);
	printf("cleaned objects\n");
}

void irc_set_user_data(irc_conn_t *c, void *data)
{
	c->user_data = data;
}

void *irc_get_user_data(irc_conn_t *c)
{
	return c->user_data;
}

