#include "value-type.hh"

namespace clap {

   ValueType::~ValueType() = default;

   void ValueType::toEngine(AudioBuffer<double> &buffer, uint32_t numFrames) const {
      struct Op {
         Op(const ValueType &vt) : _valueType(vt) {}

         double operator()(double v) const { return _valueType.toEngine(v); }

         const ValueType &_valueType;
      } op(*this);

      buffer.applyTo(op, numFrames);
   }
} // namespace clap
