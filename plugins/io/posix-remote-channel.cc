#if !(defined(__unix__) || defined(__APPLE__))
#   error "This file can only be compile for unix targets"
#endif

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#include "posix-remote-channel.hh"

namespace clap {
   PosixRemoteChannel::PosixRemoteChannel(const MessageHandler &handler,
                                          bool cookieHalf,
                                          EventControl &evControl,
                                          int socket)
      : super(handler, cookieHalf), _evControl(evControl), _socket(socket) {
      int flags = ::fcntl(_socket, F_GETFL);
      ::fcntl(_socket, F_SETFL, flags | O_NONBLOCK);

#if defined(__MACH__) && defined(__APPLE__) || defined(__FreeBSD__)
      do {
         int set = 1;
         ::setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
      } while (0);
#endif
   }

   PosixRemoteChannel::~PosixRemoteChannel() { close(); }

   void PosixRemoteChannel::tryReceive() {
      ssize_t nbytes =
         ::recv(_socket, _inputBuffer.writePtr(), _inputBuffer.writeAvail(), MSG_NOSIGNAL);
      if (nbytes < 0) {
         if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
            return;

         std::cerr << "recv(" << _socket << "): " << strerror(errno) << std::endl;

         close();
         return;
      }

      if (nbytes == 0) {
         close();
         return;
      }

      _inputBuffer.append(nbytes);
      processInput();
      _inputBuffer.rewind();
   }

   void PosixRemoteChannel::trySend() {
      while (!_outputBuffers.empty()) {
         auto &buffer = _outputBuffers.front();

         while (isOpen()) {
            auto avail = buffer.readAvail();
            if (avail == 0)
               break;

            auto nbytes = ::send(_socket, buffer.readPtr(), avail, MSG_NOSIGNAL);
            if (nbytes == -1) {
               if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
                  modifyFd(CLAP_POSIX_FD_READ | CLAP_POSIX_FD_WRITE);
                  return;
               }

               std::cerr << "send(" << _socket << "): " << strerror(errno) << std::endl;

               close();
               return;
            }

            buffer.consume(nbytes);
         }

         _outputBuffers.pop();
      }

      modifyFd(CLAP_POSIX_FD_READ);
   }

   void PosixRemoteChannel::runOnce() {
      if (!isOpen())
         return;

      pollfd pfd;
      pfd.fd = _socket;
      pfd.events = POLLIN;
      pfd.revents = 0;

      if (_ioFlags & CLAP_POSIX_FD_WRITE)
         pfd.events |= POLLOUT;
      if (_ioFlags & CLAP_POSIX_FD_ERROR)
         pfd.events |= POLLERR;

      int ret = ::poll(&pfd, 1, 10);
      if (ret == 0)
         return;

      if (ret < 0) {
         if (errno == 0 || errno == EAGAIN || errno == EINTR || errno == ETIMEDOUT)
            return;

         std::cerr << "[clap-plugins] poll(): " << strerror(errno) << std::endl;

         close();
         return;
      }

      if (pfd.revents & POLLOUT)
         trySend();
      if (isOpen() && pfd.revents & POLLIN)
         tryReceive();
      if (isOpen() && pfd.revents & POLLERR)
         close();
   }

   void PosixRemoteChannel::onError() { close(); }

   void PosixRemoteChannel::close() {
      if (_socket == -1)
         return;

      _evControl.removeFd();

      std::cerr << "closing socket(" << _socket << ")" << std::endl;
      ::close(_socket);
      _socket = -1;
   }

   void PosixRemoteChannel::modifyFd(int flags) {
      if (flags == _ioFlags)
         return;

      _ioFlags = flags;
      _evControl.modifyFd(flags);
   }
} // namespace clap
