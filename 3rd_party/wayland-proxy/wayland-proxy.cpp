/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// This code is based on Rust implementation at
// https://github.com/the8472/weyland-p5000

// Version 1.2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <spawn.h>
#include <poll.h>
#include <vector>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <memory>
#include <cassert>
#include <pthread.h>
#include <sched.h>
#include <fstream>
#include <ctime>

#include "wayland-proxy.h"

constexpr const char* stateFlags[] = {
  "WP:E ",
  "WP:D ",
  "WP:RF ",
  "WP:RT ",
  "WP:CA ",
  "WP:CR ",
  "WP:AT ",
  "WP:ACT ",
  "WP:CPCA ",
  "WP:CPCF ",
  "WP:CPSF ",
};

CompositorCrashHandler WaylandProxy::sCompositorCrashHandler = nullptr;
std::atomic<unsigned> WaylandProxy::sProxyStateFlags = 0;

// The maximum number of fds libwayland can recvmsg at once
#define MAX_LIBWAY_FDS 28
#define MAX_DATA_SIZE 4096
#define POLL_TIMEOUT 30000

bool sPrintInfo = false;

void Print(const char* aFormat, ...) {
  if (!sPrintInfo) {
    return;
  }
  va_list args;
  va_start(args, aFormat);
  vfprintf(stderr, aFormat, args);
  va_end(args);
}

void Warning(const char* aOperation) {
  fprintf(stderr, "Warning: %s : %s\n", aOperation, strerror(errno));
}

void Error(const char* aOperation) {
  fprintf(stderr, "Error: %s : %s\n", aOperation, strerror(errno));
}

void ErrorPlain(const char* aFormat, ...) {
  va_list args;
  va_start(args, aFormat);
  vfprintf(stderr, aFormat, args);
  va_end(args);
}

class WaylandMessage {
 public:
  bool Write(int aSocket);

  bool Loaded() const { return !mFailed && (mFds.size() || mData.size()); }
  bool Failed() const { return mFailed; }

  explicit WaylandMessage(int aSocket) { Read(aSocket); }
  ~WaylandMessage();

 private:
  void Read(int aSocket);

 private:
  bool mFailed = false;

  std::vector<int> mFds;
  std::vector<unsigned char> mData;
};

class ProxiedConnection {
 public:
  bool Init(int aChildSocket, char* aWaylandDisplay);
  bool IsConnected() { return mCompositorConnected; }

  struct pollfd* AddToPollFd(struct pollfd* aPfds);
  struct pollfd* LoadPollFd(struct pollfd* aPfds);

  // Process this connection (send/receive data).
  // Returns false if connection is broken and should be removed.
  bool Process();
  bool ProcessFailure();

  void PrintConnectionInfo();

  ~ProxiedConnection();

 private:
  // Try to connect to compositor. Returns false in case of fatal error.
  bool ConnectToCompositor();

  bool TransferOrQueue(
      int aSourceSocket, int aSourcePollFlags, int aDestSocket,
      std::vector<std::unique_ptr<WaylandMessage>>* aMessageQueue,
      int& aStatSent, int& aStatReceived);
  bool FlushQueue(int aDestSocket, int aDestPollFlags,
                  std::vector<std::unique_ptr<WaylandMessage>>& aMessageQueue,
                  int& aStatSent);

  // Where we should connect.
  // Weak pointer to parent WaylandProxy class.
  char* mWaylandDisplay = nullptr;

  // We don't have connected compositor yet. Try to connect
  bool mCompositorConnected = false;

  // Don't cycle endlessly over compositor connection
  int mFailedCompositorConnections = 0;
  static constexpr int sMaxFailedCompositorConnections = 100;

  // We're disconnected from app or compositor. We will close such connection.
  bool mApplicationFailed = false;
  bool mCompositorFailed = false;

  int mCompositorSocket = -1;
  int mCompositorFlags = 0;

  int mApplicationSocket = -1;
  int mApplicationFlags = 0;

  // Stored proxied data
  std::vector<std::unique_ptr<WaylandMessage>> mToCompositorQueue;
  std::vector<std::unique_ptr<WaylandMessage>> mToApplicationQueue;

  int mStatRecvFromCompositor = 0;
  int mStatSentToCompositor = 0;
  int mStatSentToCompositorLater = 0;

  int mStatRecvFromClient = 0;
  int mStatSentToClient = 0;
  int mStatSentToClientLater = 0;


  // Wait one sec before we disconnect client
  // from compositor. It gives client time to
  // process potential error messages from compositor.
  constexpr static const double sFailureTimeout = CLOCKS_PER_SEC;
  clock_t mFailureTime = 0;
};

WaylandMessage::~WaylandMessage() {
  for (auto const fd : mFds) {
    close(fd);
  }
}

void WaylandMessage::Read(int aSocket) {
  // We don't expect WaylandMessage re-read
  assert(!Loaded() && !mFailed);

  mData.resize(MAX_DATA_SIZE);

  struct msghdr msg = {0};
  struct iovec iov = {mData.data(), mData.size()};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  char cmsgdata[(CMSG_LEN(MAX_LIBWAY_FDS * sizeof(int32_t)))] = {0};
  msg.msg_control = &cmsgdata;
  msg.msg_controllen = sizeof(cmsgdata);

  ssize_t ret = recvmsg(aSocket, &msg, MSG_CMSG_CLOEXEC | MSG_DONTWAIT);
  if (msg.msg_flags & (MSG_CTRUNC | MSG_TRUNC)) {
    Error("WaylandMessage::Read() data truncated, small buffer?");
    mFailed = true;
    return;
  }

  if (ret < 1) {
    switch (errno) {
      case EAGAIN:
      case EINTR:
        // Neither loaded nor failed, we'll try again later
        Print("WaylandMessage::Read() failed %s\n", strerror(errno));
        return;
      default:
        Error("WaylandMessage::Read() failed");
        mFailed = true;
        return;
    }
  }

  // Set correct data size
  mData.resize(ret);

  // Read cmsg
  struct cmsghdr* header = CMSG_FIRSTHDR(&msg);
  while (header) {
    struct cmsghdr* next = CMSG_NXTHDR(&msg, header);
    if (header->cmsg_level != SOL_SOCKET || header->cmsg_type != SCM_RIGHTS) {
      header = next;
      continue;
    }

    int* data = (int*)CMSG_DATA(header);
    int filenum = (int)((header->cmsg_len - CMSG_LEN(0)) / sizeof(int));
    if (filenum > MAX_LIBWAY_FDS) {
      ErrorPlain("WaylandMessage::Read(): too many files to read\n");
      mFailed = true;
      return;
    }
    for (int i = 0; i < filenum; i++) {
#ifdef DEBUG
      int flags = fcntl(data[i], F_GETFL, 0);
      if (flags == -1 && errno == EBADF) {
        Error("WaylandMessage::Read() invalid fd");
      }
#endif
      mFds.push_back(data[i]);
    }
    header = next;
  }
}

bool WaylandMessage::Write(int aSocket) {
  if (!Loaded()) {
    return false;
  }

  struct msghdr msg = {0};
  struct iovec iov = {mData.data(), mData.size()};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  union {
    char buf[CMSG_SPACE(sizeof(int) * MAX_LIBWAY_FDS)];
    struct cmsghdr align;
  } cmsgu;
  memset(cmsgu.buf, 0, sizeof(cmsgu.buf));

  int filenum = mFds.size();
  if (filenum) {
    if (filenum > MAX_LIBWAY_FDS) {
      ErrorPlain("WaylandMessage::Write() too many files to send\n");
      return false;
    }
#ifdef DEBUG
    for (int i = 0; i < filenum; i++) {
      int flags = fcntl(mFds[i], F_GETFL, 0);
      if (flags == -1 && errno == EBADF) {
        Error("WaylandMessage::Write() invalid fd\n");
      }
    }
#endif
    msg.msg_control = cmsgu.buf;
    msg.msg_controllen = CMSG_SPACE(filenum * sizeof(int));

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(filenum * sizeof(int));
    memcpy(CMSG_DATA(cmsg), mFds.data(), filenum * sizeof(int));
  }

  ssize_t ret = sendmsg(aSocket, &msg, MSG_CMSG_CLOEXEC | MSG_DONTWAIT);
  if (ret < 1) {
    switch (errno) {
      case EAGAIN:
      case EINTR:
        // Neither loaded nor failed, we'll try again later
        Print("WaylandMessage::Write() failed %s\n", strerror(errno));
        return false;
      default:
        Warning("WaylandMessage::Write() failed");
        mFailed = true;
        return false;
    }
  }

  if (ret != (ssize_t)mData.size()) {
    Print("WaylandMessage::Write() failed to write all data! (%d vs. %d)\n", ret,
         mData.size());
  }
  return true;
}

ProxiedConnection::~ProxiedConnection() {
  if (mCompositorSocket != -1) {
    close(mCompositorSocket);
  }
  if (mApplicationSocket != -1) {
    close(mApplicationSocket);
  }
}

bool ProxiedConnection::Init(int aApplicationSocket, char* aWaylandDisplay) {
  mWaylandDisplay = aWaylandDisplay;
  mApplicationSocket = aApplicationSocket;
  mCompositorSocket =
      socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (mCompositorSocket == -1) {
    Error("WaylandProxy: ProxiedConnection::Init() socket()");
  }
  bool ret = mApplicationSocket >= 0 && mCompositorSocket >= 0;
  Print("WaylandProxy: ProxiedConnection::Init() %s\n", ret ? "OK" : "FAILED");
  return ret;
}

struct pollfd* ProxiedConnection::AddToPollFd(struct pollfd* aPfds) {
  // Listen application's requests
  aPfds->fd = mApplicationSocket;
  aPfds->events = POLLIN;

  // We're connected and we have data for appplication from compositor.
  // Add POLLOUT to request write to app socket.
  if (mCompositorConnected && !mToApplicationQueue.empty()) {
    aPfds->events |= POLLOUT;
  }
  aPfds++;

  aPfds->fd = mCompositorSocket;
  aPfds->events = 0;
  // We're waiting for connection or we have data for compositor
  if (!mCompositorConnected || !mToCompositorQueue.empty()) {
    aPfds->events |= POLLOUT;
  }
  if (mCompositorConnected) {
    aPfds->events |= POLLIN;
  }
  aPfds++;

  return aPfds;
}

struct pollfd* ProxiedConnection::LoadPollFd(struct pollfd* aPfds) {
  if (aPfds->fd != mApplicationSocket) {
    return aPfds;
  }
  mApplicationFlags = aPfds->revents;
  aPfds++;
  mCompositorFlags = aPfds->revents;
  aPfds++;
  return aPfds;
}

bool ProxiedConnection::ConnectToCompositor() {
  struct sockaddr_un addr = {};
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, mWaylandDisplay);

  mCompositorConnected =
      connect(mCompositorSocket, (const struct sockaddr*)&addr,
              sizeof(struct sockaddr_un)) != -1;
  if (!mCompositorConnected) {
    switch (errno) {
      case EAGAIN:
      case EALREADY:
      case ECONNREFUSED:
      case EINPROGRESS:
      case EINTR:
      case EISCONN:
      case ETIMEDOUT:
        mFailedCompositorConnections++;
        if (mFailedCompositorConnections > sMaxFailedCompositorConnections) {
          Error("ConnectToCompositor() connect() failed repeatedly");
          WaylandProxy::AddState(WAYLAND_PROXY_COMPOSITOR_SOCKET_FAILED);
          return false;
        }
        // We can recover from these errors and try again
        Warning("ConnectToCompositor() try again");
        return true;
      default:
        Error("ConnectToCompositor() connect()");
        WaylandProxy::AddState(WAYLAND_PROXY_COMPOSITOR_SOCKET_FAILED);
        return false;
    }
  }

  Print("ConnectToCompositor() Connected to compositor\n");
  WaylandProxy::AddState(WAYLAND_PROXY_COMPOSITOR_ATTACHED);
  return true;
}

// Read data from aSourceSocket and try to twite them to aDestSocket.
// If data write fails, append them to aMessageQueue.
// Return
bool ProxiedConnection::TransferOrQueue(
    int aSourceSocket, int aSourcePollFlags, int aDestSocket,
    std::vector<std::unique_ptr<WaylandMessage>>* aMessageQueue,
    int& aStatSent, int& aStatReceived) {
  // Don't read if we don't have any data ready
  if (!(aSourcePollFlags & POLLIN)) {
    return true;
  }

  while (true) {
    int availableData = 0;
    if (ioctl(aSourceSocket, FIONREAD, &availableData) < 0) {
      // Broken connection, we're finished here
      Warning("ProxiedConnection::TransferOrQueue() broken source socket\n");
      return false;
    }
    if (availableData == 0) {
      return true;
    }

    auto message = std::make_unique<WaylandMessage>(aSourceSocket);
    if (message->Failed()) {
      // Failed to read message due to error
      return false;
    }
    if (!message->Loaded()) {
      // Let's try again
      return true;
    }
    aStatReceived++;

    if (message->Write(aDestSocket)) {
      aStatSent++;
      continue;
    }
    if (message->Failed()) {
      // Failed to write and we can't recover
      return false;
    }
    aMessageQueue->push_back(std::move(message));
  }
}

// Try to flush all data to aMessageQueue.
bool ProxiedConnection::FlushQueue(
    int aDestSocket, int aDestPollFlags,
    std::vector<std::unique_ptr<WaylandMessage>>& aMessageQueue,
    int& aStatSent) {
  // Can't write to destination yet
  if (!(aDestPollFlags & POLLOUT) || aMessageQueue.empty()) {
    return true;
  }

  std::vector<std::unique_ptr<WaylandMessage>>::iterator message;
  for (message = aMessageQueue.begin(); message != aMessageQueue.end();) {
    if (!(*message)->Write(aDestSocket)) {
      // Failed to write the message, remove whole connection
      // if it's broken.
      if ((*message)->Failed()) {
        return false;
      }
      break;
    }
    aStatSent++;
    message++;
  }

  // Remove all written messages at once.
  if (message != aMessageQueue.begin()) {
    aMessageQueue.erase(aMessageQueue.begin(), message);
  }

  return true;
}

void ProxiedConnection::PrintConnectionInfo() {
  constexpr char animation[] = {'-','\\','|','/'};
  static unsigned int animationState = 0;
  animationState = (animationState + 1) % sizeof(animation);

  static bool isCharDevice = []() {
    struct stat st;
    if (fstat(STDERR_FILENO, &st)) {
      return false;
    }
    return (S_ISCHR(st.st_mode));
  }();

  fprintf(stderr,"%s[%p][%c] compositor [%d] -> [%d] delay [%d] pending [%d] | application [%d] -> [%d] delayed [%d] pending [%d]%c",
  isCharDevice ? "\r" : "",
  this,
  animation[animationState],
  mStatRecvFromCompositor,
  mStatSentToClient,
  mStatSentToClientLater,
  mStatRecvFromCompositor - mStatSentToClient - mStatSentToClientLater,
  mStatRecvFromClient,
  mStatSentToCompositor,
  mStatSentToCompositorLater,
  mStatRecvFromClient - mStatSentToCompositor - mStatSentToCompositorLater,
  isCharDevice ? ' ' : '\n');
}

bool ProxiedConnection::Process() {
  // If the connection already fails at ProxiedConnection::Process() somewhere,
  // well finish processing all pending messages and flush queues to sockets.
  // Then the connection becomes inactive (so we return early here) and we'll
  // keep application socket opened for some time and then close
  // the connection (disconnect application).
  //
  // It's because if we close application socket the app is instantly
  // terminated by gtk event loop and there may be unprocessed messages
  // pending in wayland client queues.
  //
  // That ensures we see actual wayland protocol error instead of
  // 'application is terminated' error.
  if (mApplicationFailed || mCompositorFailed) {
    return false;
  }

  // If we hit any error (instead of compositor waiting to connect),
  // we keep the code running and flush pending messages where it's possible.

  // Check if appplication is still listening
  if (mApplicationFlags & (POLLHUP | POLLERR)) {
    Print("ProxiedConnection::Process(): Client socket is not listening\n");
    WaylandProxy::AddState(WAYLAND_PROXY_APP_CONNECTION_FAILED);
    mApplicationFailed = true;
  }

  // Check if compositor is still listening
  if (mCompositorConnected) {
    if (mCompositorFlags & (POLLHUP | POLLERR)) {
      Print("ProxiedConnection::Process(): Compositor socket is not listening\n");
      WaylandProxy::AddState(WAYLAND_PROXY_COMPOSITOR_CONNECTION_FAILED);
      mCompositorFailed = true;
    }
  } else {
    // Try to reconnect to compositor.
    if (!ConnectToCompositor()) {
      Error("ProxiedConnection::Process(): Failed to connect to compositor\n");
      WaylandProxy::AddState(WAYLAND_PROXY_COMPOSITOR_CONNECTION_FAILED);
      mCompositorFailed = true;
    } else if (!mCompositorConnected) {
      // We're not connected yet but ConnectToCompositor() didn't return
      // fatal error. Try again later.
      return true;
    }
  }

  if (!TransferOrQueue(mCompositorSocket, mCompositorFlags, mApplicationSocket,
                       &mToApplicationQueue, mStatRecvFromCompositor,
                       mStatSentToClient)) {
    Error("ProxiedConnection::Process(): Failed to read data from compositor!");
      WaylandProxy::AddState(WAYLAND_PROXY_COMPOSITOR_CONNECTION_FAILED);
    mCompositorFailed = true;
  }
  if (!TransferOrQueue(mApplicationSocket, mApplicationFlags, mCompositorSocket,
                       &mToCompositorQueue, mStatRecvFromClient,
                       mStatSentToCompositor)) {
    Error("ProxiedConnection::Process(): Failed to read data from client!");
    WaylandProxy::AddState(WAYLAND_PROXY_APP_CONNECTION_FAILED);
    mApplicationFailed = true;
  }
  if (!FlushQueue(mCompositorSocket, mCompositorFlags, mToCompositorQueue,
                  mStatSentToCompositorLater)) {
    Error("ProxiedConnection::Process(): Failed to flush queue to compositor!");
    WaylandProxy::AddState(WAYLAND_PROXY_COMPOSITOR_CONNECTION_FAILED);
    mCompositorFailed = true;
  }
  if (!FlushQueue(mApplicationSocket, mApplicationFlags, mToApplicationQueue,
                  mStatSentToClientLater)) {
    Error("ProxiedConnection::Process(): Failed to flush queue to client!");
    WaylandProxy::AddState(WAYLAND_PROXY_APP_CONNECTION_FAILED);
    mApplicationFailed = true;
  }

  if (sPrintInfo) {
    PrintConnectionInfo();
  }

  if (mCompositorFailed) {
    mFailureTime = clock();
  }
  return !mApplicationFailed && !mCompositorFailed;
}

bool ProxiedConnection::ProcessFailure() {
  if (!mCompositorFailed && !mApplicationFailed) {
    return false;
  }

  if (mCompositorFailed) {
    double time = (double)(clock() - mFailureTime);
    if (time < sFailureTimeout) {
      return false;
    }

    struct stat buffer;
    if (stat(mWaylandDisplay, &buffer) < 0) {
      Print("ProxiedConnection(): compositor crashed!\n");
      WaylandProxy::CompositorCrashed();
    } else {
      Print("ProxiedConnection(): compositor fails to read/write events!\n");
    }
  } else if (mApplicationFailed) {
    Print("ProxiedConnection(): application fails to read/write events!\n");
  }

  return true;
}

bool WaylandProxy::CheckWaylandDisplay(const char* aWaylandDisplay) {
  struct sockaddr_un addr = {};
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, aWaylandDisplay);

  int sc = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (sc == -1) {
    Error("CheckWaylandDisplay(): failed to create socket");
    return false;
  }

  bool ret =
      connect(sc, (const struct sockaddr*)&addr,
              sizeof(struct sockaddr_un)) != -1;
  if (!ret) {
    switch (errno) {
      case EAGAIN:
      case EALREADY:
      case ECONNREFUSED:
      case EINPROGRESS:
      case EINTR:
      case EISCONN:
      case ETIMEDOUT:
        // We can recover from these errors and try again
        Info("CheckWaylandDisplay(): Wayland display %s non-fatal error %s\n",
             mWaylandDisplay, strerror(errno));
        ret = true;
        break;
      default:
        ErrorPlain(
          "CheckWaylandDisplay(): Failed to connect to Wayland display '%s' error: %s\n",
          mWaylandDisplay, strerror(errno));
        break;
    }
  }

  close(sc);
  return ret;
}


bool WaylandProxy::SetupWaylandDisplays() {
  char* waylandDisplay = getenv("WAYLAND_DISPLAY_COMPOSITOR");
  if (!waylandDisplay) {
    waylandDisplay = getenv("WAYLAND_DISPLAY");
    if (!waylandDisplay || waylandDisplay[0] == '\0') {
      ErrorPlain("WaylandProxy::SetupWaylandDisplays(), Missing Wayland display, WAYLAND_DISPLAY is empty.\n");
      return false;
    }
  }

  char* XDGRuntimeDir = getenv("XDG_RUNTIME_DIR");
  if (!XDGRuntimeDir) {
    ErrorPlain("WaylandProxy::SetupWaylandDisplays() Missing XDG_RUNTIME_DIR\n");
    return false;
  }

  // WAYLAND_DISPLAY can be absolute path
  if (waylandDisplay[0] == '/') {
    if (strlen(mWaylandDisplay) >= sMaxDisplayNameLen) {
      ErrorPlain("WaylandProxy::SetupWaylandDisplays() WAYLAND_DISPLAY is too large.\n");
      return false;
    }
    strcpy(mWaylandDisplay, waylandDisplay);
  } else {
    int ret = snprintf(mWaylandDisplay, sMaxDisplayNameLen, "%s/%s",
                       XDGRuntimeDir, waylandDisplay);
    if (ret < 0 || ret >= sMaxDisplayNameLen) {
      ErrorPlain("WaylandProxy::SetupWaylandDisplays() WAYLAND_DISPLAY/XDG_RUNTIME_DIR is too large.\n");
      return false;
    }
  }

  if (!CheckWaylandDisplay(mWaylandDisplay)) {
    return false;
  }

  int ret = snprintf(mWaylandProxy, sMaxDisplayNameLen,
                     "%s/wayland-proxy-%d", XDGRuntimeDir, getpid());
  if (ret < 0 || ret >= sMaxDisplayNameLen) {
    ErrorPlain("WaylandProxy::SetupWaylandDisplays() WAYLAND_DISPLAY/XDG_RUNTIME_DIR is too large.\n");
    return false;
  }

  // Save original Wayland display variable for potential reuse
  setenv("WAYLAND_DISPLAY_COMPOSITOR", waylandDisplay, /* overwrite = */ true);

  Info("SetupWaylandDisplays() Wayland '%s' proxy '%s'\n",
       mWaylandDisplay, mWaylandProxy);
  return true;
}

bool WaylandProxy::StartProxyServer() {
  mProxyServerSocket =
      socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (mProxyServerSocket == -1) {
    Error("StartProxyServer(): failed to create socket");
    return false;
  }

  struct sockaddr_un serverName = {0};
  serverName.sun_family = AF_UNIX;
  strcpy(serverName.sun_path, mWaylandProxy);

  if (bind(mProxyServerSocket, (struct sockaddr*)&serverName,
           sizeof(serverName)) == -1) {
    Error("StartProxyServer(): bind() error");
    return false;
  }
  if (listen(mProxyServerSocket, 128) == -1) {
    Error("StartProxyServer(): listen() error");
    return false;
  }

  return true;
}

bool WaylandProxy::Init() {
  Info("Init()\n");

  if (!SetupWaylandDisplays()) {
    return false;
  }

  if (!StartProxyServer()) {
    return false;
  }

  Info("Init() finished\n");
  return true;
}

void WaylandProxy::SetWaylandProxyDisplay() {
  Info("SetWaylandProxyDisplay() WAYLAND_DISPLAY %s\n", mWaylandDisplay);
  setenv("WAYLAND_DISPLAY", mWaylandProxy, /* overwrite = */ true);
}

void WaylandProxy::RestoreWaylandDisplay() {
  unlink(mWaylandProxy);
  char* waylandDisplay = getenv("WAYLAND_DISPLAY_COMPOSITOR");
  if (waylandDisplay) {
    Info("RestoreWaylandDisplay() WAYLAND_DISPLAY restored to %s\n",
         waylandDisplay);
    setenv("WAYLAND_DISPLAY", waylandDisplay, /* overwrite = */ true);
    unsetenv("WAYLAND_DISPLAY_COMPOSITOR");
  }
}

bool WaylandProxy::IsChildAppTerminated() {
  if (!mApplicationPID) {
    return false;
  }
  int status = 0;
  int ret = waitpid(mApplicationPID, &status, WNOHANG | WUNTRACED | WCONTINUED);
  if (ret == 0) {
    return false;
  }
  if (ret == mApplicationPID) {
    // Child application is terminated, so quit too.
    WaylandProxy::AddState(WAYLAND_PROXY_APP_TERMINATED);
    return true;
  }
  bool terminate = (errno == ECHILD);
  Error("IsChildAppTerminated: waitpid() error");
  return terminate;
}

bool WaylandProxy::PollConnections() {
  int nfds_max = mConnections.size() * 2 + 1;

  struct pollfd pollfds[nfds_max];
  struct pollfd* addedPollfd = pollfds;

  for (auto const& connection : mConnections) {
    addedPollfd = connection->AddToPollFd(addedPollfd);
  }
  int nfds = (addedPollfd - pollfds);

  // If all connections are attached to compositor, add another one
  // for new potential connection from application.
  bool addNewConnection = mConnections.empty() ||
                          mConnections.back()->IsConnected();
  if (addNewConnection) {
    addedPollfd->fd = mProxyServerSocket;
    addedPollfd->events = POLLIN;
    nfds++;
  }
  assert(addedPollfd < pollfds + nfds_max);

  while (1) {
    int ret = poll(pollfds, nfds, POLL_TIMEOUT);
    if (ret == 0) {
      // No change on fds
      continue;
    } else if (ret > 0) {
      // We have FD to read
      break;
    } else if (ret == -1) {
      switch (errno) {
        case EINTR:
        case EAGAIN:
          if (IsChildAppTerminated()) {
            Info("PollConnections(): ChildAppTerminated\n");
            return false;
          }
          continue;
        default:
          Error("PollConnections(): poll() error");
          return false;
      }
    }
  }

  struct pollfd* loadedPollfd = pollfds;
  for (auto const& connection : mConnections) {
    loadedPollfd = connection->LoadPollFd(loadedPollfd);
  }

  assert(loadedPollfd == addedPollfd);
  assert(loadedPollfd < pollfds + nfds_max);

  // Create a new connection if there's a new client waiting
  if (addNewConnection && (loadedPollfd->revents & POLLIN)) {
    Info("new child connection\n");
    int applicationSocket = accept4(loadedPollfd->fd, nullptr, nullptr,
                                    SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (applicationSocket == -1) {
      switch (errno) {
        case EAGAIN:
        case EINTR:
          // Try again later
          break;
        default:
          Error("Faild to accept connection from application");
          return false;
      }
    } else {
      auto connection = std::make_unique<ProxiedConnection>();
      if (connection->Init(applicationSocket, mWaylandDisplay)) {
        WaylandProxy::AddState(WAYLAND_PROXY_CONNECTION_ADDED);
        mConnections.push_back(std::move(connection));
      }
    }
  }

  return true;
}

bool WaylandProxy::ProcessConnections() {
  std::vector<std::unique_ptr<ProxiedConnection>>::iterator connection;
  for (connection = mConnections.begin(); connection != mConnections.end();) {
    if (!(*connection)->Process()) {
      WaylandProxy::AddState(WAYLAND_PROXY_CONNECTION_REMOVED);
      if ((*connection)->ProcessFailure()) {
        connection = mConnections.erase(connection);
        if (mConnections.empty()) {
          // We removed last connection - quit.
          Info("removed last connection, quit\n");
          return false;
        }
      }
    } else {
      connection++;
    }
  }
  return true;
}

void WaylandProxy::Run() {
  while (1) {
    if (IsChildAppTerminated()) {
      Info("quit - ChildAppTerminated\n");
      break;
    }
    if (!PollConnections()) {
      Info("quit - no connection\n");
      break;
    }
    if (!ProcessConnections()) {
      Info("quit - failed to process connections\n");
      break;
    }
  }
  WaylandProxy::AddState(WAYLAND_PROXY_TERMINATED);
}

WaylandProxy::~WaylandProxy() {
  Info("terminated\n");
  if (mThreadRunning) {
    Info("thread is still running, terminating.\n");
    mThreadRunning = false;
    pthread_cancel(mThread);
    pthread_join(mThread, nullptr);
  }
  if (mProxyServerSocket != -1) {
    close(mProxyServerSocket);
  }
  RestoreWaylandDisplay();
}

void* WaylandProxy::RunProxyThread(WaylandProxy* aProxy) {
#if defined(__linux__) || defined(__FreeBSD__)
  pthread_setname_np(pthread_self(), "WaylandProxy");
#endif
  aProxy->Run();
  Print("[%d] WaylandProxy [%p]: thread exited.\n", getpid(), aProxy);
  return nullptr;
}

std::unique_ptr<WaylandProxy> WaylandProxy::Create() {
  auto proxy = std::make_unique<WaylandProxy>();
  Print("[%d] WaylandProxy [%p]: Created().\n", getpid(), proxy.get());
  if (!proxy->Init()) {
    Print("[%d] WaylandProxy [%p]: Init failed, exiting.\n", getpid(), proxy.get());
    return nullptr;
  }
  return proxy;
}

bool WaylandProxy::RunChildApplication(char* argv[]) {
  if (!argv[0]) {
    ErrorPlain("WaylandProxy::RunChildApplication: missing application to run\n");
    return false;
  }

  mApplicationPID = fork();
  if (mApplicationPID == -1) {
    Error("WaylandProxy::RunChildApplication: fork() error");
    return false;
  }
  if (mApplicationPID == 0) {
    SetWaylandProxyDisplay();
    if (execv(argv[0], argv) == -1) {
      ErrorPlain(
        "WaylandProxy::RunChildApplication: failed to run %s error %s\n",
        argv[0], strerror(errno));
      exit(1);
    }
  }

  Run();
  return true;
}

bool WaylandProxy::RunThread() {
  pthread_attr_t attr;
  if (pthread_attr_init(&attr) != 0) {
    ErrorPlain("WaylandProxy::RunThread(): pthread_attr_init() failed\n");
    return false;
  }

  sched_param param;
  if (pthread_attr_getschedparam(&attr, &param) == 0) {
    param.sched_priority = sched_get_priority_min(SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &param);
  }

  SetWaylandProxyDisplay();

  mThreadRunning = pthread_create(&mThread, nullptr, (void* (*)(void*))RunProxyThread, this) == 0;
  if (!mThreadRunning) {
    ErrorPlain("WaylandProxy::RunThread(): pthread_create() failed\n");
    // If we failed to run proxy thread, set WAYLAND_DISPLAY back.
    RestoreWaylandDisplay();
    WaylandProxy::AddState(WAYLAND_PROXY_RUN_FAILED);
  }

  pthread_attr_destroy(&attr);
  return mThreadRunning;
}

void WaylandProxy::SetVerbose(bool aVerbose) { sPrintInfo = aVerbose; }

void WaylandProxy::Info(const char* aFormat, ...) {
  if (!sPrintInfo) {
    return;
  }
  fprintf(stderr,"[%d] WaylandProxy [%p]: ", getpid(), this);
  va_list args;
  va_start(args, aFormat);
  vfprintf(stderr, aFormat, args);
  va_end(args);
}

void WaylandProxy::Warning(const char* aOperation) {
  fprintf(stderr, "[%d] Wayland Proxy [%p] Warning: %s : %s\n",
          getpid(), this, aOperation, strerror(errno));
}

void WaylandProxy::Error(const char* aOperation) {
  fprintf(stderr, "[%d] Wayland Proxy [%p] Error: %s : %s\n",
          getpid(), this, aOperation, strerror(errno));
}

void WaylandProxy::ErrorPlain(const char* aFormat, ...) {
  fprintf(stderr, "[%d] Wayland Proxy [%p] Error: ", getpid(), this);
  va_list args;
  va_start(args, aFormat);
  vfprintf(stderr, aFormat, args);
  va_end(args);
}

void WaylandProxy::SetCompositorCrashHandler(CompositorCrashHandler aCrashHandler) {
  sCompositorCrashHandler = aCrashHandler;
}

void WaylandProxy::CompositorCrashed() {
  if (sCompositorCrashHandler) {
    sCompositorCrashHandler();
  }
}

void WaylandProxy::AddState(unsigned aState) {
  sProxyStateFlags.fetch_or(aState, std::memory_order_relaxed);
}

const char* WaylandProxy::GetState() {
  std::string stateString;
  unsigned state = sProxyStateFlags.load(std::memory_order_relaxed);
  for (unsigned i = 0, flag = 1; i < sizeof(stateFlags); i++, flag <<= 1) {
    if (state & flag) {
      stateString.append(stateFlags[i]);
    }
  }
  return strdup(stateString.c_str());
}
