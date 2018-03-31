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
#include <unistd.h>
#include <assert.h>
#include <zmq.h>

void test_round_robin_out (void *ctx)
{
    void *req = zmq_socket (ctx, ZMQ_REQ);
    assert (req);

    int rc = zmq_connect (req, "tcp://127.0.0.1:12345");
    rc |= zmq_connect (req, "tcp://127.0.0.1:12346");
    rc |= zmq_connect (req, "tcp://127.0.0.1:12347");
    rc |= zmq_connect (req, "tcp://127.0.0.1:12348");
    rc |= zmq_connect (req, "tcp://127.0.0.1:12349");
    assert (rc == 0);

    // Send our peer-replies, and expect every REP it used once in order
    for (size_t peer = 0; peer < 5; peer++) {
        rc = zmq_send_const (req, "ABC", 3, 0);
        assert (rc == 3);
        char buf [3];
        rc = zmq_recv (req, buf, 3, 0);
        assert (rc == 3);
        assert (memcmp (buf, "DEF", 3) == 0);
    }

    rc = zmq_close (req);
    assert (rc == 0);
}


void test_req_only_listens_to_current_peer (void *ctx)
{
    void *req = zmq_socket (ctx, ZMQ_REQ);
    assert (req);

    int rc = zmq_setsockopt (req, ZMQ_ROUTING_ID, "A", 1);
    assert (rc == 0);

    rc = zmq_connect (req, "tcp://127.0.0.1:12345");
    rc |= zmq_connect (req, "tcp://127.0.0.1:12346");
    rc |= zmq_connect (req, "tcp://127.0.0.1:12347");
    assert (rc == 0);

    // Wait for connects to finish.
    usleep ((useconds_t)300000);

    for (size_t i = 0; i < 3; ++i) {
        rc = zmq_send_const (req, "ABC", 3, 0);
        assert (rc == 3);

        // Receive only the good reply
        char buf [4];
        rc = zmq_recv (req, buf, 4, 0);
        assert (rc == 4);
        assert (memcmp (buf, "GOOD", 4) == 0);
    }

    rc = zmq_close (req);
    assert (rc == 0);
}

void test_req_message_format (void *ctx)
{
    void *req = zmq_socket (ctx, ZMQ_REQ);
    assert (req);
    int rc = zmq_connect (req, "tcp://127.0.0.1:12345");
    assert (rc == 0);

    // Send a multi-part request.
    rc = zmq_send_const (req, "ABC", 3, ZMQ_SNDMORE);
    assert (rc == 3);
    rc = zmq_send_const (req, "DEF", 3, 0);
    assert (rc == 3);

    // Receive reply.
    char buf [3];
    rc = zmq_recv (req, buf, 3, 0);
    assert (rc == 3);
    assert (memcmp (buf, "GHI", 3) == 0);

    int more = 0;
    size_t more_size = sizeof (more);
    rc = zmq_getsockopt (req, ZMQ_RCVMORE, &more, &more_size);
    assert (rc == 0);
    assert (more == 0);

    rc = zmq_close (req);
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
