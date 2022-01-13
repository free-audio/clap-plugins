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
      while (_inputBuffer.readAvail() >= 12) {
         const auto *data = _inputBuffer.readPtr();
         Message msg;

         std::memcpy(&msg.type, data, 4);
         std::memcpy(&msg.cookie, data + 4, 4);
         std::memcpy(&msg.size, data + 8, 4);
         msg.data = data + 12;

         uint32_t totalSize = 12 + msg.size;
         if (_inputBuffer.readAvail() < totalSize)
            return;

         auto it = _syncHandlers.find(msg.cookie);
         if (it != _syncHandlers.end()) {
            it->second(msg);
            _syncHandlers.erase(it);
         } else {
            _handler(msg);
         }

         _inputBuffer.read(totalSize);
      }
   }

   bool BasicRemoteChannel::sendMessageAsync(const Message &msg) {
      write(&msg.type, sizeof(msg.type));
      write(&msg.cookie, sizeof(msg.cookie));
      write(&msg.size, sizeof(msg.size));
      write(msg.data, msg.size);
      trySend();
      return true;
   }

   bool BasicRemoteChannel::sendMessageSync(const Message &msg, const MessageHandler &handler) {
      if (!sendMessageAsync(msg))
         return false;

      auto it = _syncHandlers.emplace(msg.cookie, handler);
      assert(it.second);
      if (!it.second)
         return false;

      while (isOpen() && _syncHandlers.count(msg.cookie) > 0)
         runOnce();

      _syncHandlers.erase(msg.cookie);
      return true;
   }
} // namespace clap
