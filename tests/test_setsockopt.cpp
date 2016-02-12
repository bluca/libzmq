/*
    Copyright (c) 2007-2016 Contributors as noted in the AUTHORS file

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

#include "testutil.hpp"

void test_setsockopt_tcp_recv_buffer (void)
{
    int rc;
    void *ctx = zmq_ctx_new ();
    void *socket = zmq_socket (ctx, ZMQ_PUSH);

    int val = 0;
    size_t placeholder = sizeof (val);

    rc = zmq_getsockopt (socket, ZMQ_RCVBUF, &val, &placeholder);
    assert (rc == 0);
    assert (val == 8192);

    rc = zmq_setsockopt (socket, ZMQ_RCVBUF, &val, sizeof (val));
    assert (rc == 0);
    assert (val == 8192);

    rc = zmq_getsockopt (socket, ZMQ_RCVBUF, &val, &placeholder);
    assert (rc == 0);
    assert (val == 8192);

    val = 16384;

    rc = zmq_setsockopt (socket, ZMQ_RCVBUF, &val, sizeof (val));
    assert (rc == 0);
    assert (val == 16384);

    rc = zmq_getsockopt (socket, ZMQ_RCVBUF, &val, &placeholder);
    assert (rc == 0);
    assert (val == 16384);

    zmq_close (socket);
    zmq_ctx_term (ctx);
}

void test_setsockopt_tcp_send_buffer (void)
{
    int rc;
    void *ctx = zmq_ctx_new ();
    void *socket = zmq_socket (ctx, ZMQ_PUSH);

    int val = 0;
    size_t placeholder = sizeof (val);

    rc = zmq_getsockopt (socket, ZMQ_SNDBUF, &val, &placeholder);
    assert (rc == 0);
    assert (val == 8192);

    rc = zmq_setsockopt (socket, ZMQ_SNDBUF, &val, sizeof (val));
    assert (rc == 0);
    assert (val == 8192);

    rc = zmq_getsockopt (socket, ZMQ_SNDBUF, &val, &placeholder);
    assert (rc == 0);
    assert (val == 8192);

    val = 16384;

    rc = zmq_setsockopt (socket, ZMQ_SNDBUF, &val, sizeof (val));
    assert (rc == 0);
    assert (val == 16384);

    rc = zmq_getsockopt (socket, ZMQ_SNDBUF, &val, &placeholder);
    assert (rc == 0);
    assert (val == 16384);

    zmq_close (socket);
    zmq_ctx_term (ctx);
}

void test_setsockopt_use_fd ()
{
    int rc;
    void *ctx = zmq_ctx_new ();
    void *socket = zmq_socket (ctx, ZMQ_PUSH);

    int val = 0;
    size_t placeholder = sizeof (val);

    rc = zmq_getsockopt (socket, ZMQ_USE_FD, &val, &placeholder);
    assert(rc == 0);
    assert(val == -1);

    val = 3;

    rc = zmq_setsockopt (socket, ZMQ_USE_FD, &val, sizeof(val));
    assert(rc == 0);
    assert(val == 3);

    rc = zmq_getsockopt (socket, ZMQ_USE_FD, &val, &placeholder);
    assert(rc == 0);
    assert(val == 3);

    zmq_close (socket);
    zmq_ctx_term (ctx);
}

void test_setsockopt_gen_sockopt ()
{
    int rc;
    void *ctx = zmq_ctx_new ();
    void *socket = zmq_socket (ctx, ZMQ_PUSH);

    int val_single[2] = {4, 8};
    size_t val_size = sizeof (int) * 2;

    rc = zmq_getsockopt (socket, ZMQ_SOCKOPT_ANY, val_single, &val_size);
    assert(rc == 0);
    assert(val_single[0] == 0);
    assert(val_single[1] == 0);

    val_single[0] = 16;
    val_single[1] = 32;

    val_size = sizeof (int) * 2;
    rc = zmq_setsockopt (socket, ZMQ_SOCKOPT_ANY, val_single, val_size);
    assert(rc == 0);
    assert(val_single[0] == 16);
    assert(val_single[1] == 32);

    val_single[0] = 0;
    val_single[1] = 0;

    rc = zmq_getsockopt (socket, ZMQ_SOCKOPT_ANY, val_single, &val_size);
    assert(rc == 0);
    assert(val_single[0] == 16);
    assert(val_single[1] == 32);

    int val_empty = 5;
    val_size = sizeof (int);
    rc = zmq_setsockopt (socket, ZMQ_SOCKOPT_ANY, &val_empty, val_size);
    assert (rc != 0);
    assert (val_empty == 5);

    rc = zmq_getsockopt (socket, ZMQ_SOCKOPT_ANY, &val_empty, &val_size);
    assert(rc != 0);
    assert (val_empty == 5);

    int val_multiple[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    val_size = sizeof (int) * 10;
    rc = zmq_setsockopt (socket, ZMQ_SOCKOPT_ANY, val_multiple, val_size);
    assert(rc == 0);
    for (int value = 0; value < 10; ++value)
        assert (val_multiple[value] == value);

    memset (val_multiple, 0, val_size);

    rc = zmq_getsockopt (socket, ZMQ_SOCKOPT_ANY, val_multiple, &val_size);
    assert(rc == 0);
    for (int value = 0; value < 10; ++value)
        assert (val_multiple[value] == value);

    zmq_close (socket);
    zmq_ctx_term (ctx);
}

int main (void)
{
    test_setsockopt_tcp_recv_buffer ();
    test_setsockopt_tcp_send_buffer ();
    test_setsockopt_use_fd ();
    test_setsockopt_gen_sockopt ();
}
