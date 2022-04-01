#include <cassert>

#include "parameters.hh"

namespace clap {
   Parameter *clap::Parameters::addParameter(const clap_param_info &info,
                                             const ValueType &valueType) {
      assert(_id2param.find(info.id) == _id2param.end());

      auto p = std::make_unique<Parameter>(info, valueType);
      auto ptr = p.get();
      auto ret = _id2param.insert_or_assign(info.id, p.get());
      if (!ret.second)
         throw std::logic_error("same parameter id was inserted twice");
      _params.emplace_back(std::move(p));
      return ptr;
   }

   size_t Parameters::count() const noexcept { return _params.size(); }

   Parameter *Parameters::getByIndex(size_t index) const noexcept {
      if (index > _params.size())
         return nullptr;
      return _params[index].get();
   }

   Parameter *Parameters::getById(clap_id id) const noexcept {
      auto it = _id2param.find(id);
      if (it == _id2param.end())
         return nullptr;
      return it->second;
   }
} // namespace clap
