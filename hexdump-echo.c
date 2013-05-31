/*

Copyright © 2013, Chris Barts.

This file is part of gopher.

gopher is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

gopher is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with gopher.  If not, see <http://www.gnu.org/licenses/>.  */

#include <netinet/in.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

void do_hexdump(struct evbuffer *input, struct evbuffer *output)
{
    static int i = 0;
    int len, j;
    char buf[16];

    while (evbuffer_get_length(input) >= 16) {
        evbuffer_remove(input, buf, 16);
        evbuffer_add_printf(output, "%08x: ", i);
        for (j = 0; j < 16; j++) {
            if (j % 8 == 0)
                evbuffer_add_printf(output, " ");
            evbuffer_add_printf(output, "%02x ", buf[j]);
        }

        for (j = 0; j < 16; j++) {
            if (j % 8 == 0)
                evbuffer_add_printf(output, " ");
            evbuffer_add_printf(output, "%c",
                                isgraph(buf[j]) ? buf[j] : '.');
        }

        evbuffer_add_printf(output, "\n");
        i += 16;
    }

    if ((len = evbuffer_get_length(input)) > 0) {
        evbuffer_remove(input, buf, len);
        evbuffer_add_printf(output, "%08x: ", i);

        for (j = 0; j < len; j++) {
            if (j % 8 == 0)
                evbuffer_add_printf(output, " ");
            evbuffer_add_printf(output, "%02x ", buf[j]);
        }

        for (; j < 16; j++) {
            if (j % 8 == 0)
                evbuffer_add_printf(output, " ");
            evbuffer_add_printf(output, "   ");
        }

        for (j = 0; j < len; j++) {
            if (j % 8 == 0)
                evbuffer_add_printf(output, " ");
            evbuffer_add_printf(output, "%c",
                                isgraph(buf[j]) ? buf[j] : '.');
        }

        evbuffer_add_printf(output, "\n");
    }
}

void writecb(struct bufferevent *bev, void *ctx)
{
    bufferevent_free(bev);
}

void errorcb(struct bufferevent *bev, short error, void *ctx)
{
    if (error & BEV_EVENT_ERROR) {
        perror("error: ");
    }

    bufferevent_free(bev);
}

void readcb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *input, *output;

    input = bufferevent_get_input(bev);
    output = bufferevent_get_output(bev);

    do_hexdump(input, output);

    /* http://archives.seul.org/libevent/users/Nov-2010/msg00084.html */
    bufferevent_setcb(bev, NULL, writecb, errorcb, NULL);
    bufferevent_setwatermark(bev, EV_WRITE, 0, 1);
}

void acceptcb(struct evconnlistener *listener, evutil_socket_t fd,
              struct sockaddr *address, int socklen, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev =
        bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, readcb, NULL, errorcb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

void accept_errorcb(struct evconnlistener *listener, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();

    fprintf(stderr,
            "Got an error %d (%s) on the listener. Shutting down.\n", err,
            evutil_socket_error_to_string(err));

    event_base_loopexit(base, NULL);
}

void run(void)
{
    struct evconnlistener *listener;
    struct sockaddr_in sin;
    struct event_base *base;

    if ((base = event_base_new()) == NULL) {
        fprintf(stderr, "event_base_new() error\n");
        return;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(7070);

    if ((listener =
         evconnlistener_new_bind(base, acceptcb, NULL,
                                 LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                 -1, (struct sockaddr *) &sin,
                                 sizeof(sin))) == NULL) {
        perror("Couldn't create listener: ");
        return;
    }

    evconnlistener_set_error_cb(listener, accept_errorcb);
    event_base_dispatch(base);
}

int main(int argc, char *argv[])
{
    run();
    return 0;
}
