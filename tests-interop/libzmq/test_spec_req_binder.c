/*
    Copyright (c) 2007-2018 Contributors as noted in the AUTHORS file

    This file is part of libzmq, the ZeroMQ core engine in C++.

    libzmq is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, the Contributors give you permission to link
    this library with independent modules to produce an executable,
    regardless of the license terms of these independent modules, and to
    copy and distribute the resulting executable under terms of your choice,
    provided that you also meet, for each linked independent module, the
    terms and conditions of the license of that module. An independent
    module is a module which is not derived from or based on this library.
    If you modify this library, you must extend this exception to your
    version of the library.

    libzmq is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <assert.h>
#include <zmq.h>

void test_round_robin_out (void *ctx)
{
    const size_t services = 5;
    void *rep[services];
    rep[0] = zmq_socket (ctx, ZMQ_REP);
    int rc = zmq_bind (rep[0], "tcp://127.0.0.1:12345");
    rep[1] = zmq_socket (ctx, ZMQ_REP);
    rc |= zmq_bind (rep[1], "tcp://127.0.0.1:12346");
    rep[2] = zmq_socket (ctx, ZMQ_REP);
    rc |= zmq_bind (rep[2], "tcp://127.0.0.1:12347");
    rep[3] = zmq_socket (ctx, ZMQ_REP);
    rc |= zmq_bind (rep[3], "tcp://127.0.0.1:12348");
    rep[4] = zmq_socket (ctx, ZMQ_REP);
    rc |= zmq_bind (rep[4], "tcp://127.0.0.1:12349");
    assert (rc == 0);

    // Send our peer-replies, and expect every REP it used once in order
    for (size_t peer = 0; peer < services; peer++) {
        char buf [3];
        rc = zmq_recv (rep[peer], buf, 3, 0);
        assert (rc == 3);
        assert (memcmp (buf, "ABC", 3) == 0);
        rc = zmq_send_const (rep[peer], "DEF", 3, 0);
        assert (rc == 3);
    }

    for (size_t peer = 0; peer < services; peer++) {
        rc = zmq_close (rep[peer]);
        assert (rc == 0);
    }
}


void test_req_only_listens_to_current_peer (void *ctx)
{
    int enabled = 1;
    const size_t services = 3;
    void *router[services];
    router[0] = zmq_socket (ctx, ZMQ_ROUTER);
    int rc = zmq_setsockopt (router[0], ZMQ_ROUTER_MANDATORY, &enabled,
                             sizeof (enabled));
    rc |= zmq_bind (router[0], "tcp://127.0.0.1:12345");
    router[1] = zmq_socket (ctx, ZMQ_ROUTER);
    rc |= zmq_setsockopt (router[1], ZMQ_ROUTER_MANDATORY, &enabled,
                             sizeof (enabled));
    rc |= zmq_bind (router[1], "tcp://127.0.0.1:12346");
    router[2] = zmq_socket (ctx, ZMQ_ROUTER);
    rc |= zmq_setsockopt (router[2], ZMQ_ROUTER_MANDATORY, &enabled,
                             sizeof (enabled));
    rc |= zmq_bind (router[2], "tcp://127.0.0.1:12347");
    assert (rc == 0);

    for (size_t i = 0; i < services; ++i) {
        // Receive on router i
        char buf [3];
        rc = zmq_recv (router[i], buf, 1, 0);
        assert (rc == 1);
        assert (memcmp (buf, "A", 1) == 0);
        rc = zmq_recv (router[i], buf, 1, 0);
        assert (rc == 0);
        rc = zmq_recv (router[i], buf, 3, 0);
        assert (rc == 3);
        assert (memcmp (buf, "ABC", 1) == 0);

        // Send back replies on all routers
        for (size_t j = 0; j < services; ++j) {
            const char *replies[] = {"WRONG", "GOOD"};
            const char *reply = replies[i == j ? 1 : 0];
            rc = zmq_send_const (router[j], "A", 1, ZMQ_SNDMORE);
            assert (rc == 1);
            rc = zmq_send_const (router[j], "", 0, ZMQ_SNDMORE);
            assert (rc == 0);
            rc = zmq_send_const (router[j], reply, strlen(reply), 0);
            assert (rc == strlen(reply));
        }
    }

    for (size_t i = 0; i < services; ++i) {
        rc = zmq_close (router[i]);
        assert (rc == 0);
    }
}

void test_req_message_format (void *ctx)
{
    void *router = zmq_socket (ctx, ZMQ_ROUTER);
    assert (router);
    int rc = zmq_bind (router, "tcp://127.0.0.1:12345");
    assert (rc == 0);

    zmq_msg_t peer_id_msg;
    zmq_msg_init (&peer_id_msg);

    // Receive peer routing id
    rc = zmq_msg_recv (&peer_id_msg, router, 0);
    assert (rc != -1);
    assert (zmq_msg_size (&peer_id_msg) > 0);

    int more = 0;
    size_t more_size = sizeof (more);
    rc = zmq_getsockopt (router, ZMQ_RCVMORE, &more, &more_size);
    assert (rc == 0);
    assert (more);

    // Receive the rest.
    char buf [3];
    rc = zmq_recv (router, buf, 3, 0);
    // Delimiter
    assert (rc == 0);
    rc = zmq_recv (router, buf, 3, 0);
    assert (rc == 3);
    assert (memcmp (buf, "ABC", 3) == 0);

    more = 0;
    more_size = sizeof (more);
    rc = zmq_getsockopt (router, ZMQ_RCVMORE, &more, &more_size);
    assert (rc == 0);
    assert (more);

    rc = zmq_recv (router, buf, 3, 0);
    assert (rc == 3);
    assert (memcmp (buf, "DEF", 3) == 0);

    more = 0;
    more_size = sizeof (more);
    rc = zmq_getsockopt (router, ZMQ_RCVMORE, &more, &more_size);
    assert (rc == 0);
    assert (more == 0);

    // Send back a single-part reply.
    rc = zmq_msg_send (&peer_id_msg, router, ZMQ_SNDMORE);
    assert (rc != -1);
    // Delimiter
    rc = zmq_send_const (router, "", 0, ZMQ_SNDMORE);
    assert (rc == 0);
    rc = zmq_send_const (router, "GHI", 3, 0);
    assert (rc == 3);

    rc = zmq_close (router);
    assert (rc == 0);
}

int main (int argc, char **argv)
{
    void *ctx = zmq_ctx_new ();
    assert (ctx);

    // SHALL route outgoing messages to connected peers using a round-robin
    // strategy.
    test_round_robin_out (ctx);

    // The request and reply messages SHALL have this format on the wire:
    // * A delimiter, consisting of an empty frame, added by the REQ socket.
    // * One or more data frames, comprising the message visible to the
    //   application.
    test_req_message_format (ctx);

    // SHALL accept an incoming message only from the last peer that it sent a
    // request to.
    // SHALL discard silently any messages received from other peers.
    test_req_only_listens_to_current_peer (ctx);

    int rc = zmq_ctx_term (ctx);
    assert (rc == 0);

    return 0;
}
