#pragma once

#include <string>
#include <sstream>

#include <clap/stream.h>

namespace clap {
   class ClapIStream {
   public:
      explicit ClapIStream(const clap_istream *is) : _is(is) {}

      auto read(void *s, uint64_t n) noexcept { return _is->read(_is, s, n); }

   private:
      const clap_istream * const _is;
   };

   // inefficient but convenient
   bool readAll(const clap_istream *is, std::string& data)
   {
      char buffer[4096];
      std::ostringstream os;

      while (true)
      {
         const auto nbytes = is->read(is, buffer, sizeof (buffer));
         if (nbytes < 0)
            return false;
         if (nbytes == 0) {
            data = os.str();
            return true;
         }
         os.write(buffer, nbytes);
      }
   }

   class ClapOStream {
   public:
      explicit ClapOStream(const clap_ostream *os) : _os(os) {}

      auto write(const void *s, uint64_t n) noexcept {
         return _os->write(_os, s, n);
      }

   private:
      const clap_ostream * const _os;
   };
} // namespace clap