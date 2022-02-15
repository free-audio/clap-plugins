#pragma once

#include <cstring>
#include <functional>
#include <memory>
#include <queue>
#include <unordered_map>

#include <clap/clap.h>

#include "buffer.hh"

namespace clap {
   class BasicRemoteChannel {
   public:
      class EventControl {
      public:
         virtual ~EventControl() = default;

         virtual void modifyFd(int flags) = 0;
         virtual void removeFd() = 0;
      };

      struct Message final {
         struct Header {
            Header() = default;
            Header(uint32_t cid, uint32_t cki) : cookie(cki), clientId(cid) {}
            uint32_t type = 0;
            uint32_t cookie = 0;
            uint32_t size = 0;
            uint32_t clientId = 0;
         };

         Header header;
         const void *data = nullptr;

         Message() = default;

         template <typename T>
         Message(uint32_t cid, uint32_t cki, const T &msg) : header(cid, cki) {
            set(msg);
         }

         template <typename T>
         void get(T &obj) const noexcept {
            constexpr const auto sz = sizeof(T);

            if (header.size != sz)
               std::terminate();

            if (header.type != T::type)
               std::terminate();

            std::memcpy(&obj, data, sizeof(obj));
         }

         template <typename T>
         const T &get() const noexcept {
            return *reinterpret_cast<const T *>(data);
         }

         template <typename T>
         void set(const T &msg) noexcept {
            header.type = T::type;
            data = &msg;
            header.size = sizeof(T);
         }
      };

      using MessageHandler = std::function<void(const Message &response)>;

      BasicRemoteChannel(const MessageHandler &handler, bool cookieHalf);
      virtual ~BasicRemoteChannel();

      BasicRemoteChannel(const BasicRemoteChannel &) = delete;
      BasicRemoteChannel(BasicRemoteChannel &&) = delete;
      BasicRemoteChannel &operator=(const BasicRemoteChannel &) = delete;
      BasicRemoteChannel &operator=(BasicRemoteChannel &&) = delete;

      uint32_t computeNextCookie() noexcept;

      template <typename Request>
      bool sendRequestAsync(uint32_t clientId, const Request &request) {
         return sendMessageAsync(
            BasicRemoteChannel::Message(clientId, computeNextCookie(), request));
      }

      template <typename Response>
      bool sendResponseAsync(uint32_t clientId, uint32_t cookie, const Response &response) {
         return sendMessageAsync(BasicRemoteChannel::Message(clientId, cookie, response));
      }

      template <typename Response>
      bool sendResponseAsync(const BasicRemoteChannel::Message &rqMsg, const Response &response) {
         return sendMessageAsync(
            BasicRemoteChannel::Message(rqMsg.header.clientId, rqMsg.header.cookie, response));
      }

      template <typename Request, typename Response>
      bool sendRequestSync(uint32_t clientId, const Request &request, Response &response) {
         sendMessageSync(BasicRemoteChannel::Message(clientId, computeNextCookie(), request),
                         [&response](const BasicRemoteChannel::Message &m) { m.get(response); });
         return true;
      }

      virtual void close() = 0;

      virtual bool isOpen() const noexcept = 0;

      // Called when there is data to be read, non-blocking
      virtual void tryReceive() = 0;

      // Called when data can be written, non-blocking
      virtual void trySend() = 0;

      // Called on socket exception
      virtual void onError() = 0;

   protected:
      using ReadBuffer = Buffer<uint8_t, 128 * 1024>;
      using WriteBuffer = Buffer<uint8_t, 32 * 1024>;

      virtual void runOnce() = 0;

      void write(const void *data, size_t size);
      WriteBuffer &nextWriteBuffer();

      void processInput();

      bool sendMessageAsync(const Message &msg);
      bool sendMessageSync(const Message &msg, const MessageHandler &handler);

      const bool _cookieHalf;
      uint32_t _nextCookie = 0;

      MessageHandler _handler;
      std::unordered_map<uint32_t /* cookie */, const MessageHandler &> _syncHandlers;

      ReadBuffer _inputBuffer;
      std::queue<WriteBuffer> _outputBuffers;
   };
} // namespace clap