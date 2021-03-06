#include "drake/systems/framework/output_port.h"

#include <memory>
#include <sstream>
#include <typeinfo>

#include "drake/common/autodiff.h"
#include "drake/common/default_scalars.h"
#include "drake/common/nice_type_name.h"

namespace drake {
namespace systems {

template <typename T>
std::unique_ptr<AbstractValue> OutputPort<T>::Allocate() const {
  std::unique_ptr<AbstractValue> value = DoAllocate();
  if (value == nullptr) {
    throw std::logic_error("Allocate(): allocator returned a nullptr for " +
        GetPortIdString());
  }
  DRAKE_ASSERT_VOID(CheckValidAllocation(*value));
  return value;
}

template <typename T>
void OutputPort<T>::Calc(const Context<T>& context,
                         AbstractValue* value) const {
  DRAKE_DEMAND(value != nullptr);
  DRAKE_ASSERT_VOID(system_base_.ThrowIfContextNotCompatible(context));
  DRAKE_ASSERT_VOID(CheckValidOutputType(*value));

  DoCalc(context, value);
}

template <typename T>
const AbstractValue& OutputPort<T>::Eval(const Context<T>& context) const {
  DRAKE_ASSERT_VOID(system_base_.ThrowIfContextNotCompatible(context));
  return DoEval(context);
}

template <typename T>
OutputPort<T>::OutputPort(
    const System<T>& system,
    const internal::SystemMessageInterface& system_base,
    OutputPortIndex index, PortDataType data_type, int size)
    : system_(system),
      system_base_(system_base),
      index_(index),
      data_type_(data_type),
      size_(size) {
  DRAKE_DEMAND(static_cast<const void*>(&system) == &system_base);
  if (size_ == kAutoSize) {
    DRAKE_ABORT_MSG("Auto-size ports are not yet implemented.");
  }
}

template <typename T>
std::string OutputPort<T>::GetPortIdString() const {
  std::ostringstream oss;
  oss << "output port " << this->get_index() << " of "
      << NiceTypeName::Get(system_base_) + " System " +
         system_base_.GetSystemPathname();
  return oss.str();
}

// If this is a vector-valued port, we can check that the returned abstract
// value actually holds a BasicVector-derived object, and for fixed-size ports
// that the object has the right size.
template <typename T>
void OutputPort<T>::CheckValidAllocation(const AbstractValue& proposed) const {
  if (this->get_data_type() != kVectorValued)
    return;  // Nothing we can check for an abstract port.

  auto proposed_vec = dynamic_cast<const Value<BasicVector<T>>*>(&proposed);
  if (proposed_vec == nullptr) {
    std::ostringstream oss;
    oss << "Allocate(): expected BasicVector output type but got "
        << NiceTypeName::Get(proposed) << " for " << GetPortIdString();
    throw std::logic_error(oss.str());
  }

  if (this->size() == kAutoSize)
    return;  // Any size is acceptable.

  const int proposed_size = proposed_vec->get_value().size();
  if (proposed_size != this->size()) {
    std::ostringstream oss;
    oss << "Allocate(): expected vector output type of size " << this->size()
        << " but got a vector of size " << proposed_size
        << " for " << GetPortIdString();
    throw std::logic_error(oss.str());
  }
}

// See CacheEntry::CheckValidAbstractValue; treat both methods similarly.
template <typename T>
void OutputPort<T>::CheckValidOutputType(const AbstractValue& proposed) const {
  // TODO(sherm1) Consider whether we can depend on there already being an
  //              object of this type in the output port's CacheEntryValue so we
  //              wouldn't have to allocate one here. If so could also store
  //              a precomputed type_index there for further savings. Would
  //              need to pass in a Context.
  auto good = DoAllocate();  // Expensive!
  // Attempt to interpret these as BasicVectors.
  auto proposed_vec = dynamic_cast<const Value<BasicVector<T>>*>(&proposed);
  auto good_vec = dynamic_cast<const Value<BasicVector<T>>*>(good.get());
  if (proposed_vec && good_vec) {
    CheckValidBasicVector(good_vec->get_value(),
                          proposed_vec->get_value());
  } else {
    // At least one is not a BasicVector.
    CheckValidAbstractValue(*good, proposed);
  }
}

template <typename T>
void OutputPort<T>::CheckValidAbstractValue(
    const AbstractValue& good, const AbstractValue& proposed) const {
  if (typeid(proposed) != typeid(good)) {
    std::ostringstream oss;
    oss << "Calc(): expected AbstractValue output type "
        << NiceTypeName::Get(good) << " but got " << NiceTypeName::Get(proposed)
        << " for " << GetPortIdString();
    throw std::logic_error(oss.str());
  }
}

template <typename T>
void OutputPort<T>::CheckValidBasicVector(
    const BasicVector<T>& good, const BasicVector<T>& proposed) const {
  if (typeid(proposed) != typeid(good)) {
    std::ostringstream oss;
    oss << "Calc(): expected BasicVector output type "
        << NiceTypeName::Get(good) << " but got " << NiceTypeName::Get(proposed)
        << " for " << GetPortIdString();
    throw std::logic_error(oss.str());
  }
}

// The Vector2/3 instantiations here are for the benefit of some
// older unit tests but are not otherwise advertised.
template class OutputPort<Eigen::AutoDiffScalar<Eigen::Vector2d>>;
template class OutputPort<Eigen::AutoDiffScalar<Eigen::Vector3d>>;

}  // namespace systems
}  // namespace drake

DRAKE_DEFINE_CLASS_TEMPLATE_INSTANTIATIONS_ON_DEFAULT_SCALARS(
    class ::drake::systems::OutputPort)
