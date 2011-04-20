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

#ifndef BOTIFEX_H_
#define BOTIFEX_H_

#include <glib.h>
#include "botifex_know.h"
#include "libirc.h"

void *irc_event_privmsg(irc_conn_t *c, struct irc_message *m);

struct botifex
{
	GSList *conns;
	bot_know_t *know;
	char *name;
	char *passwd;
	char *authed;
};


#endif /* BOTIFEX_H_ */
