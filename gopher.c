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
#include <sys/socket.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/thread.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <limits.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#define MAX_LINE PATH_MAX

void readcb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *input, *output;
    char *line;
    size_t n;
    int fd;
    struct stat st;

    input = bufferevent_get_input(bev);
    output = bufferevent_get_output(bev);

    line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF);

    printf("request: %s\nlength: %d\n", line, (int) n);

    if ((line == NULL) || (line[0] == '\0') || (line[0] == '\n') || (line[0] == '\r')) {
        /* Received request for selector list. */
        if ((fd = open(".selectors", O_RDONLY)) == -1) {
            evbuffer_add_printf(output,
                                "3No .selectors file. Bug administrator to fix.\t\terror.host\t1\r\n");
            goto end;
        }

        if (fstat(fd, &st) == -1) {
            close(fd);
            evbuffer_add_printf(output,
                                "3.selectors file present but unreadable.\t\terror.host\t1\r\n");
            goto end;
        }

        evbuffer_add_file(output, fd, 0, st.st_size);
    } else {
        if ((fd = open(line, O_RDONLY)) == -1) {
            evbuffer_add_printf(output,
                                "3'%s' does not exist (no handler found)\t\terror.host\t1\r\n",
                                line);
            goto end;
        }

        if (fstat(fd, &st) == -1) {
            close(fd);
            evbuffer_add_printf(output,
                                "3%s present but unreadable.\t\terror.host\t1\r\n",
                                line);
            goto end;
        }

        evbuffer_add_file(output, fd, 0, st.st_size);
    }

  end:
    free(line);
    bufferevent_free(bev);
}

void errorcb(struct bufferevent *bev, short error, void *ctx)
{
    if (error & BEV_EVENT_ERROR) {
        perror("error:");
    }

    bufferevent_free(bev);
}

void do_accept(evutil_socket_t listener, short event, void *arg)
{
    struct event_base *base = arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr *) &ss, &slen);

    if (fd < 0) {
        perror("accept");
    } else if (fd > FD_SETSIZE) {
        close(fd);
    } else {
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(fd);
        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(bev, readcb, NULL, errorcb, NULL);
        bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
        bufferevent_enable(bev, EV_READ | EV_WRITE);
    }
}

void run(void)
{
    evutil_socket_t listener;
    struct sockaddr_in sin;
    struct event *listener_event;
    struct event_base *eventBase;

    if ((eventBase = event_base_new()) == NULL) {
        fprintf(stderr, "event_base_net() error\n");
        return;
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(70);

    listener = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listener);

    if (bind(listener, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        perror("bind");
        return;
    }

    if (listen(listener, 1024) < 0) {
        perror("listen");
        return;
    }

    listener_event =
        event_new(eventBase, listener, EV_READ | EV_PERSIST, do_accept,
                  (void *) eventBase);
    event_add(listener_event, NULL);

    while (event_base_dispatch(eventBase) == 1);

    event_base_free(eventBase);
    return;
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);
    run();
    return 0;
}
