#include "core/TestSupport.hpp"
#include "core/PagedAllocator.hpp"
#include "core/DeterministicPrng.hpp"
#include "math/MathModel.hpp"
#include "math/ExpressionParser.hpp"
#include "math/Synthesizer.hpp"
#include "math/CryptoBuiltins.hpp"
#include "circuit/CircuitTopology.hpp"
#include "circuit/Solver.hpp"
#include "circuit/Virtual3PinGate.hpp"

#include <cstdio>

namespace semon {
namespace core {

namespace {

bool expect(bool cond, const char* msg) {
    if (!cond) std::printf("FAIL: %s\n", msg);
    return cond;
}

bool testCryptoMD5() {
    PagedAllocator alloc; alloc.initialize(16);
    math::MathModel* model = alloc.allocate<math::MathModel>(0, ObjectType::METADATA);
    new (model) math::MathModel(&alloc);
    model->initialize(1, "md5_test");
    std::uint32_t uid;
    model->addVariable("input", 512, &uid);

    circuit::Circuit* circuit = alloc.allocate<circuit::Circuit>(0, ObjectType::METADATA);
    new (circuit) circuit::Circuit(&alloc);
    circuit->initialize(1, "md5_circuit");

    bool ok = math::CryptoBuiltins::buildMD5(*model, "input", "md5", *circuit, &alloc);
    return expect(ok, "MD5 build") && expect(circuit->gateCount() > 0, "MD5 emits gates");
}

bool testCryptoSHA256() {
    PagedAllocator alloc; alloc.initialize(32);
    math::MathModel* model = alloc.allocate<math::MathModel>(0, ObjectType::METADATA);
    new (model) math::MathModel(&alloc);
    model->initialize(1, "sha256_test");
    std::uint32_t uid;
    model->addVariable("input", 512, &uid);

    circuit::Circuit* circuit = alloc.allocate<circuit::Circuit>(0, ObjectType::METADATA);
    new (circuit) circuit::Circuit(&alloc);
    circuit->initialize(1, "sha256_circuit");

    bool ok = math::CryptoBuiltins::buildSHA256(*model, "input", "sha256", *circuit, &alloc);
    return expect(ok, "SHA256 build") && expect(circuit->gateCount() > 0, "SHA256 emits gates");
}

bool testROBDD() {
    PagedAllocator alloc; alloc.initialize(8);
    math::ROBDDManager mgr(&alloc);
    mgr.initialize(16);

    math::ROBDDNode* a = mgr.mkNode(0, mgr.getTerminal(false), mgr.getTerminal(true));
    math::ROBDDNode* b = mgr.mkNode(1, mgr.getTerminal(false), mgr.getTerminal(true));

    math::ROBDDNode* and_node = mgr.apply(a, b, math::MathOperation::AND);
    math::ROBDDNode* or_node = mgr.apply(a, b, math::MathOperation::OR);
    math::ROBDDNode* xor_node = mgr.apply(a, b, math::MathOperation::XOR);

    bool ok = expect(and_node != nullptr, "ROBDD AND") &&
              expect(or_node != nullptr, "ROBDD OR") &&
              expect(xor_node != nullptr, "ROBDD XOR") &&
              expect(mgr.nodeCount() > 2, "ROBDD node count");

    mgr.garbageCollect();
    return ok;
}

bool testESOP() {
    PagedAllocator alloc; alloc.initialize(8);
    math::ESOPManager mgr(&alloc);
    mgr.initialize(8);

    math::ESOPExpression* a = mgr.createExpression(4);
    math::ESOPExpression* b = mgr.createExpression(4);

    bool pos[4] = {true, false, false, false}; // x0
    bool neg[4] = {false, false, false, false};
    mgr.addTerm(a, pos, neg);

    pos[0] = false; pos[1] = true; // x1
    mgr.addTerm(b, pos, neg);

    math::ESOPExpression* xor_ab = mgr.xorExpressions(a, b);
    math::ESOPExpression* and_ab = mgr.andExpressions(a, b);

    bool ok = expect(xor_ab && xor_ab->term_count == 2, "ESOP XOR terms") &&
              expect(and_ab && and_ab->term_count == 0, "ESOP AND empty (disjoint)");

    mgr.destroyExpression(a);
    mgr.destroyExpression(b);
    mgr.destroyExpression(xor_ab);
    mgr.destroyExpression(and_ab);
    return ok;
}

} // namespace

int runSelfTests() {
    bool ok = true;
    PagedAllocator alloc; ok = expect(alloc.initialize(8), "allocator init") && ok;

    circuit::Circuit* tc = alloc.allocate<circuit::Circuit>(0, ObjectType::METADATA);
    new (tc) circuit::Circuit(&alloc);
    ok = expect(tc->initialize(1, "test"), "circuit init") && ok;

    circuit::Pin* a = tc->addPin("a"); circuit::Pin* b = tc->addPin("b"); circuit::Pin* o = tc->addPin("out");
    ok = expect(a && b && o, "pins") && ok;
    ok = expect(tc->addGate(a, b, o, circuit::Virtual3PinGate::TRUTH_AND), "AND gate") && ok;
    ok = expect(tc->lockPin(a, true) && tc->lockPin(b, true), "lock") && ok;
    ok = expect(circuit::Solver::propagate(*tc), "solve") && ok;
    ok = expect(o->state == circuit::SignalState::ONE, "AND out=1") && ok;

    DeterministicPrng l, r; l.seed(12345); r.seed(12345);
    ok = expect(l.identicalTo(r), "PRNG seed") && ok;
    for (int i = 0; i < 32; ++i) ok = expect(l.nextU32() == r.nextU32(), "PRNG seq") && ok;

    math::MathModel* model = alloc.allocate<math::MathModel>(0, ObjectType::METADATA);
    new (model) math::MathModel(&alloc);
    ok = expect(model->initialize(1, "math"), "math model") && ok;
    std::uint32_t vuid; ok = expect(model->addVariable("A", 1, &vuid), "var") && ok;
    char err[128]; std::uint32_t euid; ok = expect(model->parseAndAddEquation("A == A", &euid, err, sizeof(err)), "eq parse") && ok;

    circuit::Circuit* synth = alloc.allocate<circuit::Circuit>(0, ObjectType::METADATA);
    new (synth) circuit::Circuit(&alloc);
    ok = expect(synth->initialize(2, "synth"), "synth circuit") && ok;
    ok = expect(math::Synthesizer::compileModelToCircuit(*model, *synth), "synthesize") && ok;
    ok = expect(synth->pinCount() >= 1 && synth->gateCount() >= 1, "synth topology") && ok;

    // New tests
    ok = testROBDD() && ok;
    ok = testESOP() && ok;
    ok = testCryptoMD5() && ok;
    ok = testCryptoSHA256() && ok;

    alloc.compactAll();
    std::printf(ok ? "semon self-tests passed\n" : "semon self-tests failed\n");
    return ok ? 0 : 1;
}

} // namespace core
} // namespace semon
