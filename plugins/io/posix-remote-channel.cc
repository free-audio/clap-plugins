#if !(defined(__unix__) || defined(__APPLE__))
#   error "This file can only be compile for unix targets"
#endif

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include "posix-remote-channel.hh"

namespace clap {
   PosixRemoteChannel::PosixRemoteChannel(const MessageHandler &handler,
                                          bool cookieHalf,
                                          EventControl &evControl,
                                          int socket)
      : super(handler, cookieHalf), _evControl(evControl), _socket(socket) {
      int flags = ::fcntl(_socket, F_GETFL);
      ::fcntl(_socket, F_SETFL, flags | O_NONBLOCK);
   }

   PosixRemoteChannel::~PosixRemoteChannel()
   {
      close();
   }

   void PosixRemoteChannel::tryReceive() {
      ssize_t nbytes = ::read(_socket, _inputBuffer.writePtr(), _inputBuffer.writeAvail());
      if (nbytes < 0) {
         if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
            return;

         close();
         return;
      }

      if (nbytes == 0) {
         close();
         return;
      }

      _inputBuffer.wrote(nbytes);
      processInput();
      _inputBuffer.rewind();
   }

   void PosixRemoteChannel::trySend() {
      while (!_outputBuffers.empty()) {
         auto &buffer = _outputBuffers.front();

         for (auto avail = buffer.readAvail(); avail > 0; avail = buffer.readAvail()) {
            auto nbytes = ::write(_socket, buffer.readPtr(), avail);
            if (nbytes == -1) {
               if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
                  modifyFd(CLAP_POSIX_FD_READ | CLAP_POSIX_FD_WRITE);
                  return;
               }

               close();
               return;
            }

            buffer.read(nbytes);
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
      pfd.events = POLLIN | (_ioFlags & CLAP_POSIX_FD_WRITE ? POLLOUT : 0);
      pfd.revents = 0;

      int ret = ::poll(&pfd, 1, -1);
      if (ret < 1) {
         if (errno == EAGAIN || errno == EINTR)
            return;
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