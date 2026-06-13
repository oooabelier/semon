#ifndef SEMON_CIRCUIT_CIRCUIT_TOPOLOGY_HPP
#define SEMON_CIRCUIT_CIRCUIT_TOPOLOGY_HPP

#include <cstddef>
#include <cstdint>

#include "core/PagedAllocator.hpp"
#include "core/SystemSettings.hpp"

namespace semon {
namespace circuit {

class Bus;
class Circuit;
class Virtual3PinGate;

enum class SignalState : std::uint8_t {
    FLOATING = 0U,
    ZERO = 1U,
    ONE = 2U,
    CONFLICT = 3U
};

struct Pin {
    Pin* next_in_circuit;
    Pin* previous_in_circuit;
    Circuit* parent_circuit;
    Bus* bus;
    std::uint32_t uid;
    char name[64];
    SignalState state;
    bool locked;
    bool value;
    std::uint32_t variable_uid;
    std::uint32_t bit_index;
};

class Bus {
public:
    Bus();

    bool initialize(std::uint32_t uid, const char* name, Circuit* parent);
    std::uint32_t uid() const;
    const char* name() const;
    Circuit* parentCircuit() const;
    bool addPin(Pin* pin);
    bool removePin(Pin* pin);
    Pin* pinAt(std::size_t index) const;
    std::size_t pinCount() const;
    Bus* nextInCircuit() const;
    Bus* previousInCircuit() const;
    void setNextInCircuit(Bus* next);
    void setPreviousInCircuit(Bus* previous);

private:
    std::uint32_t uid_;
    char name_[64];
    Circuit* parent_;
    Bus* next_in_circuit_;
    Bus* previous_in_circuit_;
    Pin* pins_[core::SystemSettings::MAX_BUS_PINS];
    std::size_t count_;
};

struct SubcircuitInstance {
    SubcircuitInstance* next_in_circuit;
    SubcircuitInstance* previous_in_circuit;
    std::uint32_t uid;
    char name[64];
    Circuit* blueprint;
    Circuit* parent;
    Pin* boundary_pins[core::SystemSettings::MAX_BUS_PINS];
    std::size_t boundary_count;
};

class Circuit {
public:
    explicit Circuit(core::PagedAllocator* allocator);

    bool initialize(std::uint32_t uid, const char* name, Circuit* parent = static_cast<Circuit*>(0));
    std::uint32_t uid() const;
    const char* name() const;
    Circuit* parentCircuit() const;
    core::PagedAllocator* allocator() const;

    Pin* addPin(const char* name, std::uint32_t variable_uid = 0U, std::uint32_t bit_index = 0U);
    Bus* addBus(const char* name);
    bool connect(Pin* pin, Bus* bus);
    bool disconnect(Pin* pin);
    bool lockPin(Pin* pin, bool value);
    bool unlockPin(Pin* pin);
    bool addGate(Pin* pin_a, Pin* pin_b, Pin* pin_c, std::uint8_t truth_byte, Virtual3PinGate** output_gate = static_cast<Virtual3PinGate**>(0));
    bool addSubcircuitInstance(const char* name, Circuit* blueprint, SubcircuitInstance** output_instance = static_cast<SubcircuitInstance**>(0));

    Pin* pinByUid(std::uint32_t uid) const;
    Pin* pinByIndex(std::size_t index) const;
    Pin* firstPinForSolver() const;
    Virtual3PinGate* gateByIndex(std::size_t index) const;
    Virtual3PinGate* firstGateForSolver() const;
    Bus* busByUid(std::uint32_t uid) const;
    SubcircuitInstance* subcircuitByUid(std::uint32_t uid) const;

    std::size_t pinCount() const;
    std::size_t busCount() const;
    std::size_t gateCount() const;
    std::size_t subcircuitCount() const;

private:
    bool copyName(char* destination, std::size_t destination_size, const char* source) const;
    void appendPin(Pin* pin);
    void appendGate(Virtual3PinGate* gate);
    void appendBus(Bus* bus);
    void appendSubcircuit(SubcircuitInstance* instance);

    core::PagedAllocator* allocator_;
    Circuit* parent_;
    std::uint32_t uid_;
    char name_[64];
    Pin* first_pin_;
    Pin* last_pin_;
    Virtual3PinGate* first_gate_;
    Virtual3PinGate* last_gate_;
    Bus* first_bus_;
    Bus* last_bus_;
    SubcircuitInstance* first_instance_;
    SubcircuitInstance* last_instance_;
    std::size_t pin_count_;
    std::size_t bus_count_;
    std::size_t gate_count_;
    std::size_t subcircuit_count_;
    std::uint32_t next_pin_uid_;
    std::uint32_t next_gate_uid_;
    std::uint32_t next_bus_uid_;
    std::uint32_t next_instance_uid_;
};

} // namespace circuit
} // namespace semon

#endif
