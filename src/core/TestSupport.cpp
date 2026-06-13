#include "core/TestSupport.hpp"

#include <cstdio>
#include <new>

#include "circuit/CircuitTopology.hpp"
#include "circuit/Solver.hpp"
#include "circuit/Virtual3PinGate.hpp"
#include "core/DeterministicPrng.hpp"
#include "core/PagedAllocator.hpp"
#include "math/ExpressionParser.hpp"
#include "math/MathModel.hpp"
#include "math/Synthesizer.hpp"

namespace semon {
namespace core {

namespace {

bool expect(bool condition, const char* message)
{
    if (!condition) {
        std::printf("FAIL: %s\n", message);
    }
    return condition;
}

}

int runSelfTests()
{
    bool ok = true;
    PagedAllocator allocator;
    ok = expect(allocator.initialize(8U), "allocator initializes") && ok;

    circuit::Circuit* test_circuit = allocator.allocate<circuit::Circuit>(static_cast<void*>(0), ObjectType::METADATA);
    new (test_circuit) circuit::Circuit(&allocator);
    ok = expect(test_circuit->initialize(1U, "test"), "circuit initializes") && ok;

    circuit::Pin* a = test_circuit->addPin("a");
    circuit::Pin* b = test_circuit->addPin("b");
    circuit::Pin* out = test_circuit->addPin("out");
    ok = expect(a != static_cast<circuit::Pin*>(0) && b != static_cast<circuit::Pin*>(0) &&
                    out != static_cast<circuit::Pin*>(0),
                "pins allocate") &&
         ok;
    ok = expect(test_circuit->addGate(a, b, out, circuit::Virtual3PinGate::TRUTH_AND), "AND gate adds") && ok;
    ok = expect(test_circuit->lockPin(a, true) && test_circuit->lockPin(b, true), "pins lock") && ok;
    ok = expect(circuit::Solver::propagate(*test_circuit), "solver propagates") && ok;
    ok = expect(out->state == circuit::SignalState::ONE, "AND output is one") && ok;

    DeterministicPrng left;
    DeterministicPrng right;
    left.seed(123456789ULL);
    right.seed(123456789ULL);
    ok = expect(left.identicalTo(right), "prng seeds identical") && ok;
    for (std::uint32_t index = 0U; index < 32U; ++index) {
        ok = expect(left.nextU32() == right.nextU32(), "prng sequence deterministic") && ok;
    }

    math::MathModel* model = allocator.allocate<math::MathModel>(static_cast<void*>(0), ObjectType::METADATA);
    new (model) math::MathModel(&allocator);
    ok = expect(model->initialize(1U, "math"), "math model initializes") && ok;
    std::uint32_t variable_uid = 0U;
    ok = expect(model->addVariable("A", 1U, &variable_uid), "variable adds") && ok;
    char error[128];
    std::uint32_t equation_uid = 0U;
    ok = expect(model->parseAndAddEquation("A == A", &equation_uid, error, sizeof(error)), "equation parses") && ok;

    circuit::Circuit* synthesized = allocator.allocate<circuit::Circuit>(static_cast<void*>(0), ObjectType::METADATA);
    new (synthesized) circuit::Circuit(&allocator);
    ok = expect(synthesized->initialize(2U, "synth"), "synthesis circuit initializes") && ok;
    ok = expect(semon::math::Synthesizer::compileModelToCircuit(*model, *synthesized), "model synthesizes") && ok;
    ok = expect(synthesized->pinCount() >= 1U && synthesized->gateCount() >= 1U, "synthesis emits topology") && ok;

    allocator.compactAll();
    std::printf(ok ? "semon self-tests passed\n" : "semon self-tests failed\n");
    return ok ? 0 : 1;
}

} // namespace core
} // namespace semon
