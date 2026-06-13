#include "circuit/CircuitTopology.hpp"

#include <cstring>

#include "circuit/Virtual3PinGate.hpp"

namespace semon {
namespace circuit {

Bus::Bus()
    : uid_(0U),
      name_(),
      parent_(static_cast<Circuit*>(0)),
      next_in_circuit_(static_cast<Bus*>(0)),
      previous_in_circuit_(static_cast<Bus*>(0)),
      pins_(),
      count_(0U)
{
}

bool Bus::initialize(std::uint32_t uid, const char* name, Circuit* parent)
{
    if (parent == static_cast<Circuit*>(0)) {
        return false;
    }

    uid_ = uid;
    std::size_t index = 0U;
    while ((index + 1U) < sizeof(name_) && name != static_cast<const char*>(0) && name[index] != '\0') {
        name_[index] = name[index];
        index += 1U;
    }
    name_[index] = '\0';
    parent_ = parent;
    count_ = 0U;
    for (std::size_t pin_index = 0U; pin_index < core::SystemSettings::MAX_BUS_PINS; ++pin_index) {
        pins_[pin_index] = static_cast<Pin*>(0);
    }
    return true;
}

std::uint32_t Bus::uid() const
{
    return uid_;
}

const char* Bus::name() const
{
    return name_;
}

Circuit* Bus::parentCircuit() const
{
    return parent_;
}

bool Bus::addPin(Pin* pin)
{
    if (pin == static_cast<Pin*>(0) || count_ >= core::SystemSettings::MAX_BUS_PINS) {
        return false;
    }
    for (std::size_t index = 0U; index < count_; ++index) {
        if (pins_[index] == pin) {
            return true;
        }
    }
    pins_[count_] = pin;
    count_ += 1U;
    return true;
}

bool Bus::removePin(Pin* pin)
{
    if (pin == static_cast<Pin*>(0)) {
        return false;
    }

    for (std::size_t index = 0U; index < count_; ++index) {
        if (pins_[index] == pin) {
            for (std::size_t move = index + 1U; move < count_; ++move) {
                pins_[move - 1U] = pins_[move];
            }
            pins_[count_ - 1U] = static_cast<Pin*>(0);
            count_ -= 1U;
            return true;
        }
    }
    return false;
}

Pin* Bus::pinAt(std::size_t index) const
{
    if (index >= count_) {
        return static_cast<Pin*>(0);
    }
    return pins_[index];
}

std::size_t Bus::pinCount() const
{
    return count_;
}

Bus* Bus::nextInCircuit() const
{
    return next_in_circuit_;
}

Bus* Bus::previousInCircuit() const
{
    return previous_in_circuit_;
}

void Bus::setNextInCircuit(Bus* next)
{
    next_in_circuit_ = next;
}

void Bus::setPreviousInCircuit(Bus* previous)
{
    previous_in_circuit_ = previous;
}

Circuit::Circuit(core::PagedAllocator* allocator)
    : allocator_(allocator),
      parent_(static_cast<Circuit*>(0)),
      uid_(0U),
      name_(),
      first_pin_(static_cast<Pin*>(0)),
      last_pin_(static_cast<Pin*>(0)),
      first_gate_(static_cast<Virtual3PinGate*>(0)),
      last_gate_(static_cast<Virtual3PinGate*>(0)),
      first_bus_(static_cast<Bus*>(0)),
      last_bus_(static_cast<Bus*>(0)),
      first_instance_(static_cast<SubcircuitInstance*>(0)),
      last_instance_(static_cast<SubcircuitInstance*>(0)),
      pin_count_(0U),
      bus_count_(0U),
      gate_count_(0U),
      subcircuit_count_(0U),
      next_pin_uid_(1U),
      next_gate_uid_(1U),
      next_bus_uid_(1U),
      next_instance_uid_(1U)
{
}

bool Circuit::initialize(std::uint32_t uid, const char* name, Circuit* parent)
{
    if (!copyName(name_, sizeof(name_), name)) {
        return false;
    }

    uid_ = uid;
    parent_ = parent;
    first_pin_ = static_cast<Pin*>(0);
    last_pin_ = static_cast<Pin*>(0);
    first_gate_ = static_cast<Virtual3PinGate*>(0);
    last_gate_ = static_cast<Virtual3PinGate*>(0);
    first_bus_ = static_cast<Bus*>(0);
    last_bus_ = static_cast<Bus*>(0);
    first_instance_ = static_cast<SubcircuitInstance*>(0);
    last_instance_ = static_cast<SubcircuitInstance*>(0);
    pin_count_ = 0U;
    bus_count_ = 0U;
    gate_count_ = 0U;
    subcircuit_count_ = 0U;
    next_pin_uid_ = 1U;
    next_gate_uid_ = 1U;
    next_bus_uid_ = 1U;
    next_instance_uid_ = 1U;
    return true;
}

std::uint32_t Circuit::uid() const
{
    return uid_;
}

const char* Circuit::name() const
{
    return name_;
}

Circuit* Circuit::parentCircuit() const
{
    return parent_;
}

core::PagedAllocator* Circuit::allocator() const
{
    return allocator_;
}

Pin* Circuit::addPin(const char* name, std::uint32_t variable_uid, std::uint32_t bit_index)
{
    if (allocator_ == static_cast<core::PagedAllocator*>(0)) {
        return static_cast<Pin*>(0);
    }

    Pin* pin = allocator_->allocate<Pin>(this, core::ObjectType::PIN);
    if (pin == static_cast<Pin*>(0)) {
        return static_cast<Pin*>(0);
    }

    pin->next_in_circuit = static_cast<Pin*>(0);
    pin->previous_in_circuit = static_cast<Pin*>(0);
    pin->parent_circuit = this;
    pin->bus = static_cast<Bus*>(0);
    pin->uid = next_pin_uid_;
    (void)copyName(pin->name, sizeof(pin->name), name);
    pin->state = SignalState::FLOATING;
    pin->locked = false;
    pin->value = false;
    pin->variable_uid = variable_uid;
    pin->bit_index = bit_index;
    next_pin_uid_ += 1U;
    appendPin(pin);
    return pin;
}

Bus* Circuit::addBus(const char* name)
{
    if (allocator_ == static_cast<core::PagedAllocator*>(0)) {
        return static_cast<Bus*>(0);
    }

    Bus* bus = allocator_->allocate<Bus>(this, core::ObjectType::BUS);
    if (bus == static_cast<Bus*>(0)) {
        return static_cast<Bus*>(0);
    }

    if (!bus->initialize(next_bus_uid_, name, this)) {
        return static_cast<Bus*>(0);
    }
    next_bus_uid_ += 1U;
    appendBus(bus);
    return bus;
}

bool Circuit::connect(Pin* pin, Bus* bus)
{
    if (pin == static_cast<Pin*>(0) || bus == static_cast<Bus*>(0)) {
        return false;
    }
    if (!bus->addPin(pin)) {
        return false;
    }
    pin->bus = bus;
    return true;
}

bool Circuit::disconnect(Pin* pin)
{
    if (pin == static_cast<Pin*>(0) || pin->bus == static_cast<Bus*>(0)) {
        return false;
    }

    Bus* bus = pin->bus;
    const bool removed = bus->removePin(pin);
    if (removed) {
        pin->bus = static_cast<Bus*>(0);
    }
    return removed;
}

bool Circuit::lockPin(Pin* pin, bool value)
{
    if (pin == static_cast<Pin*>(0)) {
        return false;
    }
    pin->locked = true;
    pin->value = value;
    pin->state = value ? SignalState::ONE : SignalState::ZERO;
    return true;
}

bool Circuit::unlockPin(Pin* pin)
{
    if (pin == static_cast<Pin*>(0)) {
        return false;
    }
    pin->locked = false;
    pin->state = SignalState::FLOATING;
    return true;
}

bool Circuit::addGate(Pin* pin_a,
                      Pin* pin_b,
                      Pin* pin_c,
                      std::uint8_t truth_byte,
                      Virtual3PinGate** output_gate)
{
    if (allocator_ == static_cast<core::PagedAllocator*>(0)) {
        return false;
    }

    Virtual3PinGate* gate = allocator_->allocate<Virtual3PinGate>(this, core::ObjectType::GATE);
    if (gate == static_cast<Virtual3PinGate*>(0)) {
        return false;
    }

    if (!gate->initialize(next_gate_uid_, pin_a, pin_b, pin_c, truth_byte, this)) {
        return false;
    }
    next_gate_uid_ += 1U;
    appendGate(gate);
    if (output_gate != static_cast<Virtual3PinGate**>(0)) {
        *output_gate = gate;
    }
    return true;
}

bool Circuit::addSubcircuitInstance(const char* name, Circuit* blueprint, SubcircuitInstance** output_instance)
{
    if (allocator_ == static_cast<core::PagedAllocator*>(0) || blueprint == static_cast<Circuit*>(0)) {
        return false;
    }

    SubcircuitInstance* instance = allocator_->allocate<SubcircuitInstance>(this, core::ObjectType::SUBCIRCUIT);
    if (instance == static_cast<SubcircuitInstance*>(0)) {
        return false;
    }

    instance->uid = next_instance_uid_;
    (void)copyName(instance->name, sizeof(instance->name), name);
    instance->blueprint = blueprint;
    instance->parent = this;
    instance->boundary_count = 0U;
    for (std::size_t index = 0U; index < core::SystemSettings::MAX_BUS_PINS; ++index) {
        instance->boundary_pins[index] = static_cast<Pin*>(0);
    }
    next_instance_uid_ += 1U;
    appendSubcircuit(instance);
    if (output_instance != static_cast<SubcircuitInstance**>(0)) {
        *output_instance = instance;
    }
    return true;
}

Pin* Circuit::pinByUid(std::uint32_t uid) const
{
    for (Pin* pin = first_pin_; pin != static_cast<Pin*>(0); pin = pin->next_in_circuit) {
        if (pin->uid == uid) {
            return pin;
        }
    }
    return static_cast<Pin*>(0);
}

Pin* Circuit::pinByIndex(std::size_t index) const
{
    Pin* pin = first_pin_;
    for (std::size_t current = 0U; pin != static_cast<Pin*>(0); pin = pin->next_in_circuit, ++current) {
        if (current == index) {
            return pin;
        }
    }
    return static_cast<Pin*>(0);
}

Pin* Circuit::firstPinForSolver() const
{
    return first_pin_;
}

Virtual3PinGate* Circuit::gateByIndex(std::size_t index) const
{
    Virtual3PinGate* gate = first_gate_;
    for (std::size_t current = 0U; gate != static_cast<Virtual3PinGate*>(0); gate = gate->nextInCircuit(), ++current) {
        if (current == index) {
            return gate;
        }
    }
    return static_cast<Virtual3PinGate*>(0);
}

Virtual3PinGate* Circuit::firstGateForSolver() const
{
    return first_gate_;
}

Bus* Circuit::busByUid(std::uint32_t uid) const
{
    for (Bus* bus = first_bus_; bus != static_cast<Bus*>(0); bus = bus->nextInCircuit()) {
        if (bus->uid() == uid) {
            return bus;
        }
    }
    return static_cast<Bus*>(0);
}

SubcircuitInstance* Circuit::subcircuitByUid(std::uint32_t uid) const
{
    for (SubcircuitInstance* instance = first_instance_; instance != static_cast<SubcircuitInstance*>(0); instance = instance->next_in_circuit) {
        if (instance->uid == uid) {
            return instance;
        }
    }
    return static_cast<SubcircuitInstance*>(0);
}

std::size_t Circuit::pinCount() const
{
    return pin_count_;
}

std::size_t Circuit::busCount() const
{
    return bus_count_;
}

std::size_t Circuit::gateCount() const
{
    return gate_count_;
}

std::size_t Circuit::subcircuitCount() const
{
    return subcircuit_count_;
}

bool Circuit::copyName(char* destination, std::size_t destination_size, const char* source) const
{
    if (destination == static_cast<char*>(0) || destination_size == 0U) {
        return false;
    }

    std::size_t index = 0U;
    while ((index + 1U) < destination_size && source != static_cast<const char*>(0) && source[index] != '\0') {
        destination[index] = source[index];
        index += 1U;
    }
    destination[index] = '\0';
    return true;
}

void Circuit::appendPin(Pin* pin)
{
    if (pin == static_cast<Pin*>(0)) {
        return;
    }
    if (last_pin_ == static_cast<Pin*>(0)) {
        first_pin_ = pin;
        last_pin_ = pin;
    } else {
        last_pin_->next_in_circuit = pin;
        pin->previous_in_circuit = last_pin_;
        last_pin_ = pin;
    }
    pin_count_ += 1U;
}

void Circuit::appendGate(Virtual3PinGate* gate)
{
    if (gate == static_cast<Virtual3PinGate*>(0)) {
        return;
    }
    if (last_gate_ == static_cast<Virtual3PinGate*>(0)) {
        first_gate_ = gate;
        last_gate_ = gate;
    } else {
        last_gate_->setNextInCircuit(gate);
        gate->setPreviousInCircuit(last_gate_);
        last_gate_ = gate;
    }
    gate_count_ += 1U;
}

void Circuit::appendBus(Bus* bus)
{
    if (bus == static_cast<Bus*>(0)) {
        return;
    }
    if (last_bus_ == static_cast<Bus*>(0)) {
        first_bus_ = bus;
        last_bus_ = bus;
    } else {
        last_bus_->setNextInCircuit(bus);
        bus->setPreviousInCircuit(last_bus_);
        last_bus_ = bus;
    }
    bus_count_ += 1U;
}

void Circuit::appendSubcircuit(SubcircuitInstance* instance)
{
    if (instance == static_cast<SubcircuitInstance*>(0)) {
        return;
    }
    if (last_instance_ == static_cast<SubcircuitInstance*>(0)) {
        first_instance_ = instance;
        last_instance_ = instance;
    } else {
        last_instance_->next_in_circuit = instance;
        instance->previous_in_circuit = last_instance_;
        last_instance_ = instance;
    }
    subcircuit_count_ += 1U;
}

} // namespace circuit
} // namespace semon
