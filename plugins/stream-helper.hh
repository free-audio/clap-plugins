#pragma once

#include <clap/stream.h>

namespace clap {
   class ClapIStream {
   public:
      explicit ClapIStream(clap_istream *is) : _is(is) {}

      auto read(void *s, uint64_t n) noexcept { return _is->read(_is, s, n); }

   private:
      clap_istream * const _is;
   };

   class ClapOStream {
   public:
      explicit ClapOStream(clap_ostream *os) : _os(os) {}

      auto write(const void *s, uint64_t n) noexcept {
         return _os->write(_os, s, n);
      }

   private:
      clap_ostream * const _os;
   };
} // namespace clap