#ifdef _WIN32

#   include <stdexcept>
#   include <unordered_map>
#   include <mutex>
#   include <cassert>

#   include  "win32-timer.hh"

namespace clap {

   static std::mutex s_timersLock;
   static std::unordered_map<UINT_PTR, Win32Timer*> s_timers;

   Win32Timer::Win32Timer(uint32_t durationMs, const std::function<void()> &callback)
      : _durationMs(durationMs), _cb(callback) {
      if (!_cb)
         throw std::invalid_argument("callback can't be null");
   }

   void Win32Timer::start() {
      assert(!_timer);
      _timer = SetTimer(nullptr, 0, _durationMs, &Win32Timer::onTimer);

      if (_timer) {
         std::lock_guard<std::mutex> guard(s_timersLock);
         auto it = s_timers.emplace(_timer, this);
         assert(it.second);
      }
   }

   void Win32Timer::stop() {
      if (_timer) {
         {
            std::lock_guard<std::mutex> guard(s_timersLock);
            s_timers.erase(_timer);
         }
         KillTimer(nullptr, _timer);
         _timer = 0;
      }
   }

   Win32Timer::~Win32Timer() {
      stop();
   }

   void Win32Timer::onTimer(HWND hwnd,
                       UINT message,
                       UINT_PTR idTimer,
                       DWORD dwTime) {
      Win32Timer *self = nullptr;
      {
         std::lock_guard<std::mutex> guard(s_timersLock);
         auto it = s_timers.find(idTimer);
         if (it == s_timers.end()) {
            assert(false && "the timer did fire after being killed?!");
            return;
         }
         self = it->second;
      }

      if (self)
         self->_cb();
   }
} // namespace clap

#endif