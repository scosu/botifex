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

#ifndef LIBIRC_H_
#define LIBIRC_H_

#include <glib.h>
#include <gio/gio.h>

struct _irc_conn_t;
typedef struct _irc_conn_t irc_conn_t;

enum {
	IRC_ADMIN = 0,
	IRC_AWAY,
	IRC_CONNECT,
	IRC_ERROR,
	IRC_INFO,
	IRC_INVITE,
	IRC_ISON,
	IRC_JOIN,
	IRC_KICK,
	IRC_KILL,
	IRC_LINKS,
	IRC_LIST,
	IRC_MODE,
	IRC_NAMES,
	IRC_NICK,
	IRC_NOTICE,
	IRC_OPER,
	IRC_PART,
	IRC_PASS,
	IRC_PING,
	IRC_PONG,
	IRC_PRIVMSG,
	IRC_QUIT,
	IRC_REHASH,
	IRC_RESTART,
	IRC_SERVER,
	IRC_SQUIT,
	IRC_STATS,
	IRC_SUMMON,
	IRC_TIME,
	IRC_TOPIC,
	IRC_TRACE,
	IRC_USER,
	IRC_USERHOST,
	IRC_USERS,
	IRC_VERSION,
	IRC_WALLOPS,
	IRC_WHO,
	IRC_WHOIS,
	IRC_WHOWAS,
	IRC_NUM_CMDS
};

struct irc_message {
	const char *source;
	const char *cmd;
	const char *middle;
	const char *suffix;
};

struct irc_callbacks {
	void (*admin)(irc_conn_t *, struct irc_message *);
	void (*away)(irc_conn_t *, struct irc_message *);
	void (*connect)(irc_conn_t *, struct irc_message *);
	void (*error)(irc_conn_t *, struct irc_message *);
	void (*info)(irc_conn_t *, struct irc_message *);
	void (*invite)(irc_conn_t *, struct irc_message *);
	void (*ison)(irc_conn_t *, struct irc_message *);
	void (*join)(irc_conn_t *, struct irc_message *);
	void (*kick)(irc_conn_t *, struct irc_message *);
	void (*kill)(irc_conn_t *, struct irc_message *);
	void (*links)(irc_conn_t *, struct irc_message *);
	void (*list)(irc_conn_t *, struct irc_message *);
	void (*mode)(irc_conn_t *, struct irc_message *);
	void (*names)(irc_conn_t *, struct irc_message *);
	void (*nick)(irc_conn_t *, struct irc_message *);
	void (*notice)(irc_conn_t *, struct irc_message *);
	void (*oper)(irc_conn_t *, struct irc_message *);
	void (*part)(irc_conn_t *, struct irc_message *);
	void (*pass)(irc_conn_t *, struct irc_message *);
	void (*ping)(irc_conn_t *, struct irc_message *);
	void (*pong)(irc_conn_t *, struct irc_message *);
	void (*privmsg)(irc_conn_t *, struct irc_message *);
	void (*quit)(irc_conn_t *, struct irc_message *);
	void (*rehash)(irc_conn_t *, struct irc_message *);
	void (*restart)(irc_conn_t *, struct irc_message *);
	void (*server)(irc_conn_t *, struct irc_message *);
	void (*squit)(irc_conn_t *, struct irc_message *);
	void (*stats)(irc_conn_t *, struct irc_message *);
	void (*summon)(irc_conn_t *, struct irc_message *);
	void (*time)(irc_conn_t *, struct irc_message *);
	void (*topic)(irc_conn_t *, struct irc_message *);
	void (*trace)(irc_conn_t *, struct irc_message *);
	void (*user)(irc_conn_t *, struct irc_message *);
	void (*userhost)(irc_conn_t *, struct irc_message *);
	void (*users)(irc_conn_t *, struct irc_message *);
	void (*version)(irc_conn_t *, struct irc_message *);
	void (*wallops)(irc_conn_t *, struct irc_message *);
	void (*who)(irc_conn_t *, struct irc_message *);
	void (*whois)(irc_conn_t *, struct irc_message *);
	void (*whowas)(irc_conn_t *, struct irc_message *);
};

struct _irc_conn_t {
	GSocketClient *sock;
	GSocketConnection *conn;
	GInputStream *in;
	GOutputStream *out;
	GThread *thread;

	void *user_data;

	struct irc_callbacks callbacks;

	int shutdown;
};

irc_conn_t *irc_connect(const char *host_port, const char *nick, const char *name,
		const char *passwd, struct irc_callbacks *callbacks, void *data);
void irc_set_user_data(irc_conn_t *c, void *data);
void *irc_get_user_data(irc_conn_t *c);
void irc_set_nick(irc_conn_t *c, const char *nick);
void irc_join_channel(irc_conn_t *c, const char *channel);
void irc_leave_channel(irc_conn_t *c, const char *channel);
void irc_disconnect(irc_conn_t *c, const char *msg);
void irc_privmsg(irc_conn_t *c, const char *dst, const char *msg);

#endif /* LIBIRC_H_ */
