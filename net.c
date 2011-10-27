/* net.c - stupid boilerplate shit that I shouldn't have to write */

/* Copyright (C) 2007 Keith Rarick and Philotic Inc.

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "net.h"
#include "util.h"

static int listen_socket = -1;
static struct event listen_evq;
static evh accept_handler;
static usec main_deadline = 0;
static int brakes_are_on = 1, after_startup = 0;

int
make_server_socket(char *host, char *port)
{
    int fd, flags, r;
    struct linger linger = {0, 0};
    struct addrinfo *airoot, *ai, hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    r = getaddrinfo(host, port, &hints, &airoot);
    if (r == -1)
      return twarn("getaddrinfo()"), -1;

    for(ai = airoot; ai; ai = ai->ai_next) {
      fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
      if (fd == -1) {
        twarn("socket()");
        continue;
      }

      flags = fcntl(fd, F_GETFL, 0);
      if (flags < 0) {
        twarn("getting flags");
        close(fd);
        continue;
      }

      r = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
      if (r == -1) {
        twarn("setting O_NONBLOCK");
        close(fd);
        continue;
      }

      flags = 1;
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof flags);
      setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof flags);
      setsockopt(fd, SOL_SOCKET, SO_LINGER,   &linger, sizeof linger);
      setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof flags);

      r = bind(fd, ai->ai_addr, ai->ai_addrlen);
      if (r == -1) {
        twarn("bind()");
        close(fd);
        continue;
      }

      r = listen(fd, 1024);
      if (r == -1) {
        twarn("listen()");
        close(fd);
        continue;
      }

      break;
    }

    freeaddrinfo(airoot);

    if(ai == NULL)
      fd = -1;

    return listen_socket = fd;
}

void
brake()
{
    int r;

    if (brakes_are_on) return;
    brakes_are_on = 1;
    twarnx("too many connections; putting on the brakes");

    r = event_del(&listen_evq);
    if (r == -1) twarn("event_del()");

    r = listen(listen_socket, 0);
    if (r == -1) twarn("listen()");
}

void
unbrake(evh h)
{
    int r;

    if (!brakes_are_on) return;
    brakes_are_on = 0;
    if (after_startup) twarnx("releasing the brakes");
    after_startup = 1;

    accept_handler = h ? : accept_handler;
    event_set(&listen_evq, listen_socket, EV_READ | EV_PERSIST,
              accept_handler, &listen_evq);

    set_main_timeout(main_deadline);

    r = listen(listen_socket, 1024);
    if (r == -1) twarn("listen()");
}

void
set_main_timeout(usec deadline_at)
{
    int r;
    struct timeval tv;
    usec now = now_usec();

    main_deadline = deadline_at;

    /* If there is no deadline, we just wait for a long while.
     * This works around a bug in libevent or kqueue on Mac OS X. */
    if (!deadline_at) deadline_at = now + 100 * SECOND;

    timeval_from_usec(&tv, (deadline_at > now) ? deadline_at - now : 1);

    r = event_add(&listen_evq, &tv);
    if (r == -1) twarn("event_add()");
}
