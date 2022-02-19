#ifdef __unix__
#   include <errno.h>
#   include <fcntl.h>
#   include <poll.h>
#   include <unistd.h>
#endif

#include "remote-channel.hh"

namespace clap {

   BasicRemoteChannel::BasicRemoteChannel(const MessageHandler &handler, bool cookieHalf)
      : _cookieHalf(cookieHalf), _handler(handler) {}

   BasicRemoteChannel::~BasicRemoteChannel() {}

   BasicRemoteChannel::WriteBuffer &BasicRemoteChannel::nextWriteBuffer() {
      if (_outputBuffers.empty()) {
         _outputBuffers.emplace();
         return _outputBuffers.back();
      }

      auto &buffer = _outputBuffers.back();
      if (buffer.writeAvail() > 0)
         return buffer;

      _outputBuffers.emplace();
      return _outputBuffers.back();
   }

   void BasicRemoteChannel::write(const void *_data, size_t size) {
      const uint8_t *data = static_cast<const uint8_t *>(_data);
      while (size > 0) {
         auto &buffer = nextWriteBuffer();
         buffer.write(data, size);
      }

      assert(size == 0);
   }

   uint32_t BasicRemoteChannel::computeNextCookie() noexcept {
      uint32_t cookie = _nextCookie;
      if (_cookieHalf)
         cookie |= (1ULL << 31);
      else
         cookie &= ~(1ULL << 31);

      ++_nextCookie; // overflow is fine
      return cookie;
   }

   void BasicRemoteChannel::processInput() {
      while (_inputBuffer.readAvail() >= sizeof (Message::Header)) {
         const auto *data = _inputBuffer.readPtr();
         Message msg;

         std::memcpy(&msg.header, data, sizeof (msg.header));
         msg.data = data + sizeof (msg.header);

         uint32_t totalSize = sizeof (msg.header) + msg.header.size;
         if (_inputBuffer.readAvail() < totalSize)
            return;

         auto it = _syncHandlers.find(msg.header.cookie);
         if (it != _syncHandlers.end()) {
            it->second(msg);
            _syncHandlers.erase(it);
         } else {
            _handler(msg);
         }

         _inputBuffer.consume(totalSize);
      }
   }

   bool BasicRemoteChannel::sendMessageAsync(const Message &msg) {
      write(&msg.header, sizeof(msg.header));
      write(msg.data, msg.header.size);
      trySend();
      return true;
   }

   bool BasicRemoteChannel::sendMessageSync(const Message &msg, const MessageHandler &handler) {
      if (!sendMessageAsync(msg))
         return false;

      auto it = _syncHandlers.emplace(msg.header.cookie, handler);
      assert(it.second);
      if (!it.second)
         return false;

      while (isOpen() && _syncHandlers.count(msg.header.cookie) > 0)
         runOnce();

      _syncHandlers.erase(msg.header.cookie);
      return true;
   }
} // namespace clap
