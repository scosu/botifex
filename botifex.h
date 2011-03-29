/*
 * botifex.h
 *
 *  Created on: Mar 28, 2011
 *      Author: scosu
 */

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
