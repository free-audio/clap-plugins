#include "../value-types/decibel-value-type.hh"

#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace {

   CATCH_TEST_CASE("value-type - decibel") {
      clap::DecibelValueType vt;

      CATCH_CHECK_THAT(vt.toParam(vt.toEngine(3)), Catch::Matchers::WithinAbs(3, 1e-6));
   }

} // namespace
