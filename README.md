`gopher`: A simple example of how to use `libevent2` by way of a Gopher server
==============================================================================

This is an example of how to write a trivial server in `libevent2` that nonetheless looks a little like the kinds of servers people might actually need to write in the real world. It implements the Gopher protocol, because `libevent2` already has so much support for HTTP that the simplest libevent2 HTTP server doesn't illustrate very much.

* http://tools.ietf.org/html/rfc1436
* http://en.wikipedia.org/wiki/Gopher_%28protocol%29
