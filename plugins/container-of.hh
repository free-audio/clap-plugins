#pragma once

#include <cstddef>
#include <cstdint>

namespace clap {
   /**
    * Returns the address of the object of type T containing the given member at the address u.
    */
   template <typename T, typename U>
   T *containerOf(U *u, U T::*Member) noexcept {

      union {
         T *obj;
         uint8_t *obj2;
      };
      union {
         U *mb;
         uint8_t *mb2;
      };

      obj = nullptr;
      mb = &(obj->*Member);
      auto offset = mb2 - obj2;
      mb = u;
      obj2 = mb2 - offset;
      return obj;
   }
} // namespace clap
