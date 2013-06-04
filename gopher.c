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
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#define MAX_LINE PATH_MAX

void do_gopher(char *line, struct evbuffer *output)
{
    char buf[BUFSIZ], *end;
    FILE *proc;
    struct stat ss;
    size_t len;
    int fd;

    if ((line[0] == '\n') || (line[0] == '\r') || (line[0] == '\t')) {
        /* Received request for selector list. */
        if ((fd = open(".selectors", O_RDONLY)) == -1) {
            evbuffer_add_printf(output,
                                "3No .selectors file. Bug administrator to fix.\tfake\terror.host\t1\r\n.\r\n");
            goto end;
        }

    } else if (!strncmp(line, "fortune", 7)) {
        if ((proc = popen("/usr/games/fortune", "r")) == NULL) {
            evbuffer_add_printf(output,
                                "3popen() failed: %s.\tfake\terror.host\t1\r\n.\r\n",
                                strerror(errno));
            goto end;
        }

        while ((len = fread(buf, 1, BUFSIZ, proc)) > 0) {
            evbuffer_add(output, buf, len);
        }

        pclose(proc);

        goto end;

    } else {
        if ((end = memchr(line, '\r', MAX_LINE))
            || (end = memchr(line, '\n', MAX_LINE))) {
            *end = '\0';
        } else {
            evbuffer_add_printf(output,
                                "3Malformed request\tfake\terror.host\t1\r\n.\r\n");
            goto end;
        }

        if ((fd = open(line, O_RDONLY)) == -1) {
            evbuffer_add_printf(output,
                                "3'%s' does not exist (no handler found)\tfake\terror.host\t1\r\n.\r\n",
                                line);
            goto end;
        }

    }

    if (fstat(fd, &ss) == -1) {
        evbuffer_add_printf(output, "3fstat failed\tfake\terror.host\t1\r\n.\r\n");
        close(fd);
        goto end;
    }

    evbuffer_add_file(output, fd, 0, ss.st_size);

  end:
    return;
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
    size_t inlen;
    char line[MAX_LINE];

    input = bufferevent_get_input(bev);
    output = bufferevent_get_output(bev);

    inlen = evbuffer_get_length(input);

    if (inlen >= MAX_LINE) {
        evbuffer_remove(input, line, MAX_LINE);
        evbuffer_drain(input, inlen - MAX_LINE);
    } else {
        evbuffer_remove(input, line, inlen);
    }

    do_gopher(line, output);

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
    sin.sin_port = htons(70);

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
