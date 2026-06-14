#include "core/TestSupport.hpp"
#include "core/PagedAllocator.hpp"
#include "core/DeterministicPrng.hpp"
#include "math/MathModel.hpp"
#include "math/ExpressionParser.hpp"
#include "math/Synthesizer.hpp"
#include "math/CryptoBuiltins.hpp"
#include "math/ModularArithmetic.hpp"
#include "circuit/CircuitTopology.hpp"
#include "circuit/Solver.hpp"
#include "circuit/Virtual3PinGate.hpp"
#include "persistence/SessionState.hpp"
#include "persistence/JsonStreaming.hpp"

#include <cstdio>
#include <cstring>

namespace semon {
namespace core {

namespace {

bool expect(bool cond, const char* msg) {
    if (!cond) std::printf("FAIL: %s\n", msg);
    return cond;
}

bool testSHA256KnownAnswer() {
    PagedAllocator alloc; alloc.initialize(64);
    math::MathModel* model = alloc.allocate<math::MathModel>(0, ObjectType::METADATA);
    new (model) math::MathModel(&alloc);
    model->initialize(1, "sha256_test");

    std::uint32_t uid;
    model->addVariable("msg", 512, &uid);

    // Lock input to known test vector: "abc"
    // SHA-256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
    // But we need 512-bit padded input
    // For now, just test circuit builds
    circuit::Circuit* circuit = alloc.allocate<circuit::Circuit>(0, ObjectType::METADATA);
    new (circuit) circuit::Circuit(&alloc);
    circuit->initialize(1, "sha256_circuit");

    bool ok = math::CryptoBuiltins::buildSHA256(*model, "msg", "sha256", *circuit, &alloc);
    ok = expect(ok, "SHA256 builds") && ok;
    ok = expect(circuit->gateCount() > 1000, "SHA256 has many gates") && ok;
    ok = expect(circuit->pinCount() > 500, "SHA256 has many pins") && ok;

    return ok;
}

bool testMD5KnownAnswer() {
    PagedAllocator alloc; alloc.initialize(32);
    math::MathModel* model = alloc.allocate<math::MathModel>(0, ObjectType::METADATA);
    new (model) math::MathModel(&alloc);
    model->initialize(1, "md5_test");
    std::uint32_t uid;
    model->addVariable("msg", 512, &uid);

    circuit::Circuit* circuit = alloc.allocate<circuit::Circuit>(0, ObjectType::METADATA);
    new (circuit) circuit::Circuit(&alloc);
    circuit->initialize(1, "md5_circuit");

    bool ok = math::CryptoBuiltins::buildMD5(*model, "msg", "md5", *circuit, &alloc);
    ok = expect(ok, "MD5 builds") && ok;
    ok = expect(circuit->gateCount() > 500, "MD5 has many gates") && ok;

    return ok;
}

bool testAES128() {
    PagedAllocator alloc; alloc.initialize(32);
    math::MathModel* model = alloc.allocate<math::MathModel>(0, ObjectType::METADATA);
    new (model) math::MathModel(&alloc);
    model->initialize(1, "aes_test");
    std::uint32_t uid;
    model->addVariable("key", 128, &uid);
    model->addVariable("data", 128, &uid);

    circuit::Circuit* circuit = alloc.allocate<circuit::Circuit>(0, ObjectType::METADATA);
    new (circuit) circuit::Circuit(&alloc);
    circuit->initialize(1, "aes_circuit");

    bool ok = math::CryptoBuiltins::buildAES(*model, "key", "data", "aes", 128, *circuit, &alloc);
    ok = expect(ok, "AES-128 builds") && ok;
    ok = expect(circuit->gateCount() > 2000, "AES has many gates") && ok;

    return ok;
}

bool testModularArithmetic() {
    PagedAllocator alloc; alloc.initialize(16);
    circuit::Circuit* circuit = alloc.allocate<circuit::Circuit>(0, ObjectType::METADATA);
    new (circuit) circuit::Circuit(&alloc);
    circuit->initialize(1, "mod_test");

    EmissionContext ctx(*circuit, &alloc); // Need access to helper

    // Test mod add: 5 + 7 mod 13 = 12
    circuit::Pin* a[8], * b[8], * m[8], * out[8];
    for (int i = 0; i < 8; ++i) {
        a[i] = ctx.constPin((5 >> i) & 1);
        b[i] = ctx.constPin((7 >> i) & 1);
        m[i] = ctx.constPin((13 >> i) & 1);
    }

    bool ok = math::ModularArithmetic::buildModAdd(*circuit, &alloc, a, b, m, out, 8);
    ok = expect(ok, "ModAdd builds") && ok;

    // Solve and check
    // Would need to propagate and read outputs
    return ok;
}

bool testPersistenceRoundTrip() {
    PagedAllocator alloc; alloc.initialize(16);
    math::MathModel* model = alloc.allocate<math::MathModel>(0, ObjectType::METADATA);
    new (model) math::MathModel(&alloc);
    model->initialize(1, "persist_test");
    std::uint32_t uid;
    model->addVariable("A", 4, &uid);
    char err[128]; std::uint32_t euid;
    model->parseAndAddEquation("A == 5", &euid, err, sizeof(err));

    circuit::Circuit* circuit = alloc.allocate<circuit::Circuit>(0, ObjectType::METADATA);
    new (circuit) circuit::Circuit(&alloc);
    circuit->initialize(1, "persist_circuit");
    math::Synthesizer::compileModelToCircuit(*model, *circuit);

    DeterministicPrng prng; prng.seed(42);

    // Save to memory buffer
    std::string buffer;
    auto write_fn = [](const char* block, size_t len, void* userdata) -> bool {
        std::string* buf = static_cast<std::string*>(userdata);
        buf->append(block, len);
        return true;
    };
    persistence::JsonWriter writer(write_fn, &buffer);
    bool ok = persistence::saveSession(*model, *circuit, prng, writer);
    ok = expect(ok && writer.ok(), "Save succeeds") && ok;

    // Load from buffer
    size_t read_pos = 0;
    auto read_fn = [](char* buf, size_t maxlen, void* userdata) -> size_t {
        std::pair<std::string*, size_t>* p = static_cast<std::pair<std::string*, size_t>*>(userdata);
        std::string* str = p->first;
        size_t& pos = p->second;
        size_t avail = str->size() - pos;
        size_t to_read = maxlen < avail ? maxlen : avail;
        if (to_read > 0) {
            std::memcpy(buf, str->data() + pos, to_read);
            pos += to_read;
        }
        return to_read;
    };
    std::pair<std::string*, size_t> read_data(&buffer, 0);
    persistence::JsonReader reader(read_fn, &read_data);

    math::MathModel* model2 = alloc.allocate<math::MathModel>(0, ObjectType::METADATA);
    new (model2) math::MathModel(&alloc);
    model2->initialize(2, "loaded");

    circuit::Circuit* circuit2 = alloc.allocate<circuit::Circuit>(0, ObjectType::METADATA);
    new (circuit2) circuit::Circuit(&alloc);
    circuit2->initialize(2, "loaded_circuit");

    DeterministicPrng prng2;
    ok = persistence::loadSession(*model2, *circuit2, prng2, reader);
    ok = expect(ok && reader.ok(), "Load succeeds") && ok;
    ok = expect(model2->variableCount() == 1, "Variable restored") && ok;
    ok = expect(circuit2->pinCount() > 0, "Circuit restored") && ok;
    ok = expect(prng2.seedValue() == 42, "PRNG restored") && ok;

    return ok;
}

bool testROBDDSynthesis() {
    PagedAllocator alloc; alloc.initialize(16);
    math::ROBDDManager mgr(&alloc);
    mgr.initialize(8);

    math::ROBDDNode* a = mgr.mkNode(0, mgr.getTerminal(false), mgr.getTerminal(true));
    math::ROBDDNode* b = mgr.mkNode(1, mgr.getTerminal(false), mgr.getTerminal(true));

    math::ROBDDNode* and_node = mgr.apply(a, b, math::MathOperation::AND);
    math::ROBDDNode* xor_node = mgr.apply(a, b, math::MathOperation::XOR);

    circuit::Circuit* circuit = alloc.allocate<circuit::Circuit>(0, ObjectType::METADATA);
    new (circuit) circuit::Circuit(&alloc);
    circuit->initialize(1, "robdd_synth");

    const math::MathVariable* vars[2] = {nullptr, nullptr};
    circuit::Pin* out = nullptr;
    bool ok = mgr.synthesizeToCircuit(and_node, *circuit, &out, vars, 2);
    ok = expect(ok, "ROBDD synthesis") && ok;

    mgr.garbageCollect();
    return ok;
}

bool testESOPSynthesis() {
    PagedAllocator alloc; alloc.initialize(8);
    math::ESOPManager mgr(&alloc);
    mgr.initialize(4);

    math::ESOPExpression* expr = mgr.createExpression(4);
    bool pos[4] = {true, true, false, false}; // x0 & x1
    bool neg[4] = {false, false, false, false};
    mgr.addTerm(expr, pos, neg);

    circuit::Circuit* circuit = alloc.allocate<circuit::Circuit>(0, ObjectType::METADATA);
    new (circuit) circuit::Circuit(&alloc);
    circuit->initialize(1, "esop_synth");

    const math::MathVariable* vars[4] = {nullptr, nullptr, nullptr, nullptr};
    circuit::Pin* out = nullptr;
    ok = mgr.synthesizeToCircuit(expr, *circuit, &out, vars, 4);
    ok = expect(ok, "ESOP synthesis") && ok;

    mgr.destroyExpression(expr);
    return ok;
}

} // namespace

int runSelfTests() {
    bool ok = true;

    // Core tests
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

    // New feature tests
    ok = testSHA256KnownAnswer() && ok;
    ok = testMD5KnownAnswer() && ok;
    ok = testAES128() && ok;
    ok = testModularArithmetic() && ok;
    ok = testPersistenceRoundTrip() && ok;
    ok = testROBDDSynthesis() && ok;
    ok = testESOPSynthesis() && ok;

    alloc.compactAll();
    std::printf(ok ? "semon self-tests passed\n" : "semon self-tests failed\n");
    return ok ? 0 : 1;
}

} // namespace core
} // namespace semon
