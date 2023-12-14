#include "../value-types/decibel-value-type.hh"

#include <catch2/catch_all.hpp>

namespace {

   CATCH_TEST_CASE("value-type - decibel") {
      clap::DecibelValueType vt;

      CATCH_CHECK(vt.toParam(vt.toEngine(3)) == 3);
   }

} // namespace