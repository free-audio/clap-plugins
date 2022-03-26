#pragma once

#include <cstdint>
#include <string>

#include <clap/clap.h>

#include "parameter.hh"

namespace clap {
   class CorePlugin;
   class Module {
   public:
      Module(const Module &) = delete;
      Module(Module &&) = delete;
      Module &operator=(const Module &) = delete;
      Module &operator=(Module &&) = delete;

      Module(CorePlugin& plugin, std::string name, clap_id paramIdStart);
      virtual ~Module();

      // This value is fixed and must **NEVER** change.
      // It defines the amount of space reserved for clap parameters id within
      // this module.
      static const constexpr uint32_t parameter_capacity = 1024;

   protected:
      CorePlugin &_plugin;
      const std::string _name;
      const clap_id _paramIdStart;
   };
} // namespace clap
