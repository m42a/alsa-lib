/*
 *  Sequencer Interface - middle-level routines
 *
 *  Copyright (c) 1999 by Takashi Iwai <tiwai@suse.de>
 *
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include "seq_local.h"

/**
 * \brief queue controls - start/stop/continue
 * \param seq sequencer handle
 * \param q queue id to control
 * \param type event type
 * \param value event value
 * \param ev event instance
 *
 * This function sets up general queue control event and sends it.
 * To send at scheduled time, set the schedule in \a ev.
 * If \a ev is NULL, the event is composed locally and sent immediately
 * to the specified queue.  In any cases, you need to call #snd_seq_drain_event
 * apropriately to feed the event.
 */
int snd_seq_control_queue(snd_seq_t *seq, int q, int type, int value, snd_seq_event_t *ev)
{
	snd_seq_event_t tmpev;
	if (ev == NULL) {
		snd_seq_ev_clear(&tmpev);
		ev = &tmpev;
		snd_seq_ev_set_direct(ev);
	}
	snd_seq_ev_set_queue_control(ev, type, q, value);
	return snd_seq_event_output(seq, ev);
}


/**
 * \brief create a port - simple version
 * \param seq sequencer handle
 * \param name the name of the port
 * \param caps capability bits
 * \param type type bits
 * \return the created port number or negative error code
 *
 * Creates a port with the given capability and type bits.
 */
int snd_seq_create_simple_port(snd_seq_t *seq, const char *name,
			       unsigned int caps, unsigned int type)
{
	snd_seq_port_info_t pinfo;
	int result;

	memset(&pinfo, 0, sizeof(pinfo));
	if (name)
		strncpy(pinfo.name, name, sizeof(pinfo.name) - 1);
	pinfo.capability = caps;
	pinfo.type = type;
	pinfo.midi_channels = 16;
	pinfo.midi_voices = 64; /* XXX */
	pinfo.synth_voices = 0; /* XXX */

	result = snd_seq_create_port(seq, &pinfo);
	if (result < 0)
		return result;
	else
		return pinfo.addr.port;
}

/**
 * \brief delete the port
 * \param seq sequencer handle
 * \param port port id
 * \return 0 on success or negavie error code
 */
int snd_seq_delete_simple_port(snd_seq_t *seq, int port)
{
	return snd_seq_delete_port(seq, port);
}

/**
 * \brief simple subscription (w/o exclusive & time conversion)
 * \param myport the port id as receiver
 * \param src_client sender client id
 * \param src_port sender port id
 * \return 0 on success or negative error code
 *
 * Connect from the given sender client:port to the given destination port in the
 * current client.
 */
int snd_seq_connect_from(snd_seq_t *seq, int myport, int src_client, int src_port)
{
	snd_seq_port_subscribe_t subs;
	
	memset(&subs, 0, sizeof(subs));
	subs.sender.client = src_client;
	subs.sender.port = src_port;
	/*subs.dest.client = seq->client;*/
	subs.dest.client = snd_seq_client_id(seq);
	subs.dest.port = myport;

	return snd_seq_subscribe_port(seq, &subs);
}

/**
 * \brief sipmle subscription (w/o exclusive & time conversion)
 * \param myport the port id as sender
 * \param dest_client destination client id
 * \param dest_port destination port id
 * \return 0 on success or negative error code
 *
 * Connect from the given receiver port in the current client
 * to the given destination client:port.
 */
int snd_seq_connect_to(snd_seq_t *seq, int myport, int dest_client, int dest_port)
{
	snd_seq_port_subscribe_t subs;
	
	memset(&subs, 0, sizeof(subs));
	/*subs.sender.client = seq->client;*/
	subs.sender.client = snd_seq_client_id(seq);
	subs.sender.port = myport;
	subs.dest.client = dest_client;
	subs.dest.port = dest_port;

	return snd_seq_subscribe_port(seq, &subs);
}

/**
 * \brief simple disconnection
 * \param myport the port id as receiver
 * \param src_client sender client id
 * \param src_port sender port id
 * \return 0 on success or negative error code
 *
 * Remove connection from the given sender client:port
 * to the given destination port in the current client.
 */
int snd_seq_disconnect_from(snd_seq_t *seq, int myport, int src_client, int src_port)
{
	snd_seq_port_subscribe_t subs;
	
	memset(&subs, 0, sizeof(subs));
	subs.sender.client = src_client;
	subs.sender.port = src_port;
	/*subs.dest.client = seq->client;*/
	subs.dest.client = snd_seq_client_id(seq);
	subs.dest.port = myport;

	return snd_seq_unsubscribe_port(seq, &subs);
}

/**
 * \brief simple disconnection
 * \param myport the port id as sender
 * \param dest_client destination client id
 * \param dest_port destination port id
 * \return 0 on success or negative error code
 *
 * Remove connection from the given sender client:port
 * to the given destination port in the current client.
 */
int snd_seq_disconnect_to(snd_seq_t *seq, int myport, int dest_client, int dest_port)
{
	snd_seq_port_subscribe_t subs;
	
	memset(&subs, 0, sizeof(subs));
	/*subs.sender.client = seq->client;*/
	subs.sender.client = snd_seq_client_id(seq);
	subs.sender.port = myport;
	subs.dest.client = dest_client;
	subs.dest.port = dest_port;

	return snd_seq_unsubscribe_port(seq, &subs);
}

/*
 * set client information
 */

/**
 * \brief set client name
 * \param seq sequencer handle
 * \param name name string
 * \return 0 on success or negative error code
 */
int snd_seq_set_client_name(snd_seq_t *seq, const char *name)
{
	snd_seq_client_info_t info;
	int err;

	if ((err = snd_seq_get_client_info(seq, &info)) < 0)
		return err;
	strncpy(info.name, name, sizeof(info.name) - 1);
	return snd_seq_set_client_info(seq, &info);
}

/**
 * \brief add client event filter
 * \param seq sequencer handle
 * \param event_type event type to be added
 * \return 0 on success or negative error code
 */
int snd_seq_set_client_event_filter(snd_seq_t *seq, int event_type)
{
	snd_seq_client_info_t info;
	int err;

	if ((err = snd_seq_get_client_info(seq, &info)) < 0)
		return err;
	info.filter |= SNDRV_SEQ_FILTER_USE_EVENT;
	snd_seq_set_bit(event_type, info.event_filter);
	return snd_seq_set_client_info(seq, &info);
}

/**
 * \brief change the output pool size of the given client
 * \param seq sequencer handle
 * \param size output pool size
 * \return 0 on success or negative error code
 */
int snd_seq_set_client_pool_output(snd_seq_t *seq, size_t size)
{
	snd_seq_client_pool_t info;
	int err;

	if ((err = snd_seq_get_client_pool(seq, &info)) < 0)
		return err;
	info.output_pool = size;
	return snd_seq_set_client_pool(seq, &info);
}

/**
 * \brief change the output room size of the given client
 * \param seq sequencer handle
 * \param size output room size
 * \return 0 on success or negative error code
 */
int snd_seq_set_client_pool_output_room(snd_seq_t *seq, size_t size)
{
	snd_seq_client_pool_t info;
	int err;

	if ((err = snd_seq_get_client_pool(seq, &info)) < 0)
		return err;
	info.output_room = size;
	return snd_seq_set_client_pool(seq, &info);
}

/**
 * \brief change the input pool size of the given client
 * \param seq sequencer handle
 * \param size input pool size
 * \return 0 on success or negative error code
 */
int snd_seq_set_client_pool_input(snd_seq_t *seq, size_t size)
{
	snd_seq_client_pool_t info;
	int err;

	if ((err = snd_seq_get_client_pool(seq, &info)) < 0)
		return err;
	info.input_pool = size;
	return snd_seq_set_client_pool(seq, &info);
}

/**
 * \brief reset client output pool
 * \param seq sequencer handle
 * \return 0 on success or negative error code
 */
int snd_seq_reset_pool_output(snd_seq_t *seq)
{
	struct sndrv_seq_remove_events rmp;

	memset(&rmp, 0, sizeof(rmp));
	rmp.remove_mode = SNDRV_SEQ_REMOVE_OUTPUT; /* remove all outputs */
	return snd_seq_remove_events(seq, &rmp);
}

/**
 * \brief reset client input pool
 * \param seq sequencer handle
 * \return 0 on success or negative error code
 */
int snd_seq_reset_pool_input(snd_seq_t *seq)
{
	snd_seq_remove_events_t rmp;

	memset(&rmp, 0, sizeof(rmp));
	rmp.remove_mode = SNDRV_SEQ_REMOVE_INPUT; /* remove all inputs */
	return snd_seq_remove_events(seq, &rmp);
}

/**
 * \brief parse the given string and get the sequencer address
 * \param seq sequencer handle
 * \param addr the address pointer to be returned
 * \param arg the string to be parsed
 * \return 0 on success or negative error code
 */
int snd_seq_parse_address(snd_seq_t *seq, snd_seq_addr_t *addr, const char *arg)
{
	char *p;
	int client, port;

	assert(seq && addr && arg);

	if ((p = strpbrk(arg, ":.")) == NULL)
		return -EINVAL;
	if ((port = atoi(p + 1)) < 0)
		return -EINVAL;
	addr->port = port;
	if (isdigit(*arg)) {
		client = atoi(arg);
		if (client < 0)
			return -EINVAL;
		addr->client = client;
	} else {
		/* convert from the name */
		snd_seq_client_info_t cinfo;
		int len;

		*p = 0;
		len = (int)(p - arg); /* length of client name */
		if (len <= 0)
			return -EINVAL;
		cinfo.client = -1;
		while (snd_seq_query_next_client(seq, &cinfo) >= 0) {
			if (! strncmp(cinfo.name, arg, len)) {
				addr->client = cinfo.client;
				return 0;
			}
		}
		return -ENOENT; /* not found */
	}
	return 0;
}

