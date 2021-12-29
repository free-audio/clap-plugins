#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>

#include <clap/clap.h>

#include "parameter-interpolator.hh"
#include "parameter.hh"

namespace clap {
   class Parameters {
   public:
      Parameters() = default;
      Parameters(const Parameters &parameters);

      void addParameter(const clap_param_info &info);
      void addParameter(const Parameter &param);

      size_t count() const noexcept;

      Parameter *getByIndex(size_t index) const noexcept;

      Parameter *getById(clap_id id) const noexcept;

   private:
      friend class boost::serialization::access;

      template <class Archive>
      void save(Archive &ar, const unsigned int version) const {
         std::vector<std::pair<clap_id, double>> values;
         for (auto &p : _params)
            values.emplace_back(p->info().id, p->value());

         ar << values;
      }

      template <class Archive>
      void load(Archive &ar, const unsigned int version) {
         std::vector<std::pair<clap_id, double>> values;
         ar >> values;

         for (auto &p : _params)
            p->setDefaultValue();

         for (auto &v : values) {
            auto *p = getById(v.first);
            if (!p)
               continue;
            p->setValueImmediately(v.second);
         }
      }

      BOOST_SERIALIZATION_SPLIT_MEMBER()

      std::vector<std::unique_ptr<Parameter>> _params;
      std::unordered_map<clap_id, Parameter *> _id2param;
   };

} // namespace clap

BOOST_CLASS_VERSION(clap::Parameters, 1)