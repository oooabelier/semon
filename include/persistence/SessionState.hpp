#ifndef SEMON_PERSISTENCE_SESSION_STATE_HPP
#define SEMON_PERSISTENCE_SESSION_STATE_HPP

#include "circuit/CircuitTopology.hpp"
#include "core/DeterministicPrng.hpp"
#include "math/MathModel.hpp"
#include "persistence/JsonStreaming.hpp"

namespace semon {
namespace persistence {

bool saveSession(const math::MathModel& model,
                 const circuit::Circuit& circuit,
                 const core::DeterministicPrng& prng,
                 JsonWriter& writer);

bool loadSession(math::MathModel& model,
                 circuit::Circuit& circuit,
                 core::DeterministicPrng& prng,
                 JsonReader& reader);

} // namespace persistence
} // namespace semon

#endif
