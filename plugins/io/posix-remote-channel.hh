#pragma once

#include "basic-remote-channel.hh"

namespace clap {

   class PosixRemoteChannel final : public BasicRemoteChannel {
      using super = BasicRemoteChannel;
   public:

      PosixRemoteChannel(const MessageHandler &handler,
                         bool cookieHalf,
                         EventControl &evControl,
                         int socket);
      ~PosixRemoteChannel() override;

      int fd() const noexcept { return _socket; }
      bool isOpen() const noexcept override { return _socket != -1; }

      void close() override;

      void tryReceive() override;
      void trySend() override;
      void onError() override;

   private:

      void runOnce() override;

      void modifyFd(int flags);

      EventControl &_evControl;
      int _socket;
      int _ioFlags = 0;
   };
} // namespace clap