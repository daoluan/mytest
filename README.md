# tinyco
[![Build Status](https://travis-ci.org/daoluan/tinyco.svg?branch=master)](https://travis-ci.org/daoluan/tinyco)

tinyco is a framework with **high performance and high efficiency** focusing on business with which you can build a server in only couple lines of code. It use coroutine implemented in ucontext syscall.

# Features
- High performance. Make full use of CPU and process requests concurrently which makes server's throughput greatly improved
- Perfect http/dns components
- Small kernel in size
- Business code can be easy to read and maintained. No more async framework

![!](https://github.com/daoluan/tinyco/blob/master/static/img/tinyco_perf_8c.jpg?raw=true)

# How to start ?
Clone tinyco and build it:

    % git clone https://github.com/daoluan/tinyco.git
    % cd tinyco
    % git submodule update --init --recursive

    % cmake . && make thirdparty && make && make install

It's very easy to DIY your tinyco-server. You can refer to [this](https://github.com/daoluan/tinyco/tree/master/example/server). 

For more usages, Read [here](https://github.com/daoluan/tinyco/tree/master/example).

# refs

- [how-to-solve-thundering-herd-in-tinyco](http://daoluan.net/%E7%BC%96%E7%A8%8B%E5%B0%8F%E8%AE%B0/2017/08/16/how-to-solve-thundering-herd-in-tinyco.html)
- [how-to-DIY-your-tinyco-server](http://daoluan.net/%E7%BC%96%E7%A8%8B%E5%B0%8F%E8%AE%B0/2017/09/02/how-to-DIY-your-tinyco-server.html)
