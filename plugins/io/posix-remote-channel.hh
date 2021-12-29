#pragma once

#include "basic-remote-channel.hh"

namespace clap {

   class PosixRemoteChannel final : public BasicRemoteChannel {
      using super = BasicRemoteChannel;
   public:
      class EventControl {
      public:
         virtual void modifyFd(clap_fd_flags flags) = 0;
         virtual void removeFd() = 0;
      };

      PosixRemoteChannel(const MessageHandler &handler,
                         bool cookieHalf,
                         EventControl &evControl,
                         int socket);
      ~PosixRemoteChannel() override;

      clap_fd fd() const noexcept { return _socket; }
      bool isOpen() const noexcept override { return _socket != -1; }

      void close() override;

      void tryReceive() override;
      void trySend() override;
      void onError() override;

   private:

      void runOnce() override;

      void modifyFd(clap_fd_flags flags);

      EventControl &_evControl;
      int _socket;
      clap_fd_flags _ioFlags = 0;
   };
} // namespace clap