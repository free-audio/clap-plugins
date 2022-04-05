#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <yas/serialize.hpp>
#include <yas/types/std/pair.hpp>
#include <yas/types/std/vector.hpp>

#include <clap/clap.h>

#include "parameter.hh"

namespace clap {
   class Parameters {
   public:
      Parameters() = default;
      Parameters(const Parameters &parameters);

      Parameter *addParameter(const clap_param_info &info, const ValueType& valueType = SimpleValueType::instance);

      size_t count() const noexcept;

      Parameter *getByIndex(size_t index) const noexcept;

      Parameter *getById(clap_id id) const noexcept;

      void reset()
      {
         for (auto &p : _params)
            p->setDefaultValue();
      }

      template <class Archive>
      void serialize(Archive &ar) const {
         std::vector<std::pair<clap_id, double>> values;
         for (auto &p : _params)
            values.emplace_back(p->info().id, p->value());

         ar & YAS_OBJECT(nullptr, values);
      }

      template <class Archive>
      void serialize(Archive &ar) {
         std::vector<std::pair<clap_id, double>> values;

         ar & YAS_OBJECT(nullptr, values);

         reset();

         for (auto &v : values) {
            auto *p = getById(v.first);
            if (!p)
               continue;
            p->setValueImmediately(v.second);
         }
      }

   private:
      std::vector<std::unique_ptr<Parameter>> _params;
      std::unordered_map<clap_id, Parameter *> _id2param;
   };

} // namespace clap