#include <Windows.h>

#include "win32-remote-channel.hh"

namespace clap {
   struct Win32RemoteChannelOverlapped final {
      Win32RemoteChannelOverlapped(Win32RemoteChannel &c) : remoteChannel(c) {}
      ~Win32RemoteChannelOverlapped() {
         CloseHandle(event);
      }

      OVERLAPPED overlapped{};
      Win32RemoteChannel &remoteChannel;
      HANDLE const event = CreateEvent(nullptr, false, false, nullptr);
   };

   Win32RemoteChannel::Win32RemoteChannel(const MessageHandler &handler,
                                          bool cookieHalf,
                                          void *readHandle,
                                          void *writeHandle)
      : super(handler, cookieHalf), _rHandle(readHandle), _wHandle(writeHandle),
        _rOverlapped(new Win32RemoteChannelOverlapped(*this)),
        _wOverlapped(new Win32RemoteChannelOverlapped(*this)) {
      tryReceive();
   }

   Win32RemoteChannel::~Win32RemoteChannel() { close(); }

   void Win32RemoteChannel::close() {
      if (_rHandle) {
         CloseHandle(_rHandle);
         _rHandle = nullptr;
      }

      if (_wHandle) {
         CloseHandle(_wHandle);
         _wHandle = nullptr;
      }
   }

   bool Win32RemoteChannel::isOpen() const noexcept { return _rHandle && _wHandle; }

   void Win32RemoteChannel::tryReceive() {
      if (_isReceiving)
         return;

      _isReceiving = true;
      _inputBuffer.rewind();
      if (!ReadFileEx(_rHandle,
                      _inputBuffer.writePtr(),
                      _inputBuffer.writeAvail(),
                      &_rOverlapped->overlapped,
                      &Win32RemoteChannel::receiveCompleted)) {
         _isReceiving = false;
         close();
         return;
      }
   }

   void Win32RemoteChannel::receiveCompleted(DWORD dwErrorCode,
                                             DWORD dwNumberOfBytesTransfered,
                                             LPOVERLAPPED lpOverlapped) {
      auto *o = reinterpret_cast<Win32RemoteChannelOverlapped *>(lpOverlapped);
      auto &c = o->remoteChannel;

      if (dwErrorCode == 0) {
         assert(c._isReceiving);
         c._inputBuffer.wrote(dwNumberOfBytesTransfered);
         c._isReceiving = false;
         c.processInput();
         c.tryReceive();
      } else {
         c.close();
      }
   }

   void Win32RemoteChannel::trySend() {
      if (_isSending)
         return;

      while (!_outputBuffers.empty()) {
         auto &buffer = _outputBuffers.front();
         const auto avail = buffer.readAvail();
         if (avail == 0) {
            _outputBuffers.pop();
            continue;
         }

         _isSending = true;
         if (!WriteFileEx(_wHandle,
                          buffer.readPtr(),
                          buffer.readAvail(),
                          &_wOverlapped->overlapped,
                          &Win32RemoteChannel::sendCompleted)) {
            _isSending = false;
            close();
         }
         return;
      }
   }

   void Win32RemoteChannel::sendCompleted(DWORD dwErrorCode,
                                          DWORD dwNumberOfBytesTransfered,
                                          LPOVERLAPPED lpOverlapped) {
      auto *o = reinterpret_cast<Win32RemoteChannelOverlapped *>(lpOverlapped);
      auto &c = o->remoteChannel;

      if (dwErrorCode == 0) {
         assert(c._isSending);
         auto &buffer = c._outputBuffers.front();
         buffer.read(dwNumberOfBytesTransfered);
         c._isSending = false;
         c.trySend();
      } else {
         c.close();
      }
   }

   void Win32RemoteChannel::onError() { close(); }

   void Win32RemoteChannel::runOnce() {
      trySend();
      tryReceive();

      HANDLE events[2] = {_rOverlapped->event, _wOverlapped->event};
      WaitForMultipleObjects(2, events, false, INFINITE);
   }

} // namespace clap
