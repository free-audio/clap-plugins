#pragma once

#include <clap/stream.h>

namespace clap {
   class ClapIStream {
   public:
      explicit ClapIStream(const clap_istream *is) : _is(is) {}

      auto read(void *s, uint64_t n) noexcept { return _is->read(_is, s, n); }

   private:
      const clap_istream * const _is;
   };

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