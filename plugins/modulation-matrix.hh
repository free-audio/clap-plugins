#pragma once

namespace clap {
   class Voice;
   class Parameter;

   // TODO
   class ModulationMatrix {
   public:
      double getModulation(Voice &voice, Parameter &param);
   };
} // namespace clap