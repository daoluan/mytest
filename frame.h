#ifndef FRAME_H_
#define FRAME_H_

#include <inttypes.h>
#include <unordered_map>
#include <event2/event.h>
#include <list>
#include <vector>
#include <memory>
#include <fcntl.h>
#include <unistd.h>

#include "thread.h"
#include "work.h"
#include "mutex.h"

namespace tinyco {

#define LOG(fmt, arg...) \
  printf("[%s][%s][%u]: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##arg)

class IsCompleteBase {
 public:
  IsCompleteBase() {}
  virtual ~IsCompleteBase() {}

  int CheckPkg(const char *buffer, uint32_t buffer_len) { return 0; }
};

class Frame {
 public:
  static bool Init();
  static bool Fini();

 public:
  static int UdpSendAndRecv(const std::string &sendbuf,
                            struct sockaddr_in &dest_addr,
                            std::string *recvbuf);
  static int TcpSendAndRecv(const void *sendbuf, size_t sendlen, void *recvbuf,
                            size_t *recvlen, IsCompleteBase *is_complete);
  static int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
  static int connect(int sockfd, const struct sockaddr *addr,
                     socklen_t addrlen);
  static int send(int sockfd, const void *buf, size_t len, int flags);
  static int recv(int sockfd, void *buf, size_t len, int flags);
  static int sendto(int sockfd, const void *buf, size_t len, int flags,
                    const struct sockaddr *dest_addr, socklen_t addrlen);
  static ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                          struct sockaddr *src_addr, socklen_t *addrlen);
  static int CreateThread(Work *w);
  static void Sleep(uint32_t ms);
  static int Schedule();
  static void RecycleRunningThread();

  template <class W>
  static int ListenAndAccept(uint32_t ip, uint16_t port);

 private:
  static void SocketReadOrWrite(int fd, short events, void *arg);
  static int MainThreadLoop(void *arg);
  static void PendThread(Thread *t);
  static Thread *PopPendingTop();
  static void WakeupPendingThread();
  static timeval GetEventLoopTimeout();
  static int EventLoop(const timeval &tv);
  static void UpdateLoopTimestamp();
  static uint64_t GetLastLoopTimestamp() { return last_loop_ts_; }

  static std::unordered_map<int, Thread *> io_wait_map_;
  static std::list<Thread *> thread_runnable_;
  static std::list<Thread *> thread_free_;
  static std::vector<Thread *> thread_pending_;
  static Thread *main_thread_;
  static Thread *running_thread_;
  static Thread *prunning_thread_;
  static struct event_base *base;

 private:
  Frame() {}
  virtual ~Frame() {}

  static uint64_t last_loop_ts_;
};

class defer {
  std::function<void()> t;

 public:
  defer(std::function<void()> &&t) : t(t) {}
  ~defer() { t(); }
};

template <class W>
class ListenAndAcceptWork : public Work {
 public:
  ListenAndAcceptWork(uint32_t ip, uint16_t port) : ip_(ip), port_(port) {}
  virtual ~ListenAndAcceptWork() {}

  int Run() {
    auto listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
      LOG("socket error");
      return -1;
    }

    defer d([=] { close(listenfd); });

    struct sockaddr_in server, client;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = ip_;
    server.sin_port = htons(port_);

    auto flags = fcntl(listenfd, F_GETFL, 0);
    if (flags < 0) {
      LOG("fcntl get error");
      return -1;
    }

    flags = flags | O_NONBLOCK;
    fcntl(listenfd, F_SETFL, flags);

    int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
        0) {
      LOG("fail to setsockopt(SO_REUSEADDR)");
      return -1;
    }

    if (bind(listenfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
      LOG("bind error");
      return -1;
    }

    if (listen(listenfd, 5) < 0) {
      LOG("listen error");
      return -1;
    }

    auto masterpid = getpid();
    for (auto i = 0; i < 3; i++) {
      if (fork() > 0)  // parent
        break;
    }

    // try to lock when multiprocss on
    FileMtx fm;
    if (fm.OpenLockFile(std::string("/tmp/tinyco_lf_") +
                        std::to_string(masterpid)) < 0) {
      return -1;
    }

    while (true) {
      if (fm.TryLock() < 0) {
        // lock error, schedule out
        Frame::Sleep(500);
        continue;
      }

      socklen_t socklen = sizeof(client);
      auto fd = Frame::accept(listenfd, (struct sockaddr *)&client, &socklen);
      auto w = new W(fd);
      if (fd < 0) {
        continue;
      }

      fm.Unlock();
      Frame::CreateThread(w);
    }

    return 0;
  }

 private:
  uint32_t ip_;
  uint16_t port_;
};

template <class W>
int Frame::ListenAndAccept(uint32_t ip, uint16_t port) {
  Frame::CreateThread(new ListenAndAcceptWork<W>(ip, port));
  Frame::Schedule();
}
}

#endif
