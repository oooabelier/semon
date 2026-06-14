#include "core/Shell.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <readline/readline.h>
#include <readline/history.h>

#include "circuit/CircuitTopology.hpp"
#include "circuit/Virtual3PinGate.hpp"
#include "circuit/Solver.hpp"
#include "core/DeterministicPrng.hpp"
#include "core/PagedAllocator.hpp"
#include "core/Registry.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "math/ExpressionParser.hpp"
#include "math/MathModel.hpp"
#include "math/Synthesizer.hpp"
#include "math/CryptoBuiltins.hpp"
#include "persistence/JsonStreaming.hpp"
#include "persistence/SessionState.hpp"

namespace semon {
namespace core {

namespace {
PagedAllocator g_allocator;
DeterministicPrng g_prng;
math::MathModel* g_current_model = static_cast<math::MathModel*>(0);
circuit::Circuit* g_current_circuit = static_cast<circuit::Circuit*>(0);

const char* stateColor(circuit::SignalState state) {
    switch (state) {
    case circuit::SignalState::ONE: return "\033[32m";
    case circuit::SignalState::ZERO: return "\033[31m";
    case circuit::SignalState::CONFLICT: return "\033[35m";
    case circuit::SignalState::FLOATING:
    default: return "\033[33m";
    }
}

const char* stateName(circuit::SignalState state) {
    switch (state) {
    case circuit::SignalState::ONE: return "1";
    case circuit::SignalState::ZERO: return "0";
    case circuit::SignalState::CONFLICT: return "!";
    case circuit::SignalState::FLOATING:
    default: return "?";
    }
}

void resetColor() { std::cout << "\033[0m"; }

bool writeToFile(const char* block, std::size_t length, void* userdata) {
    std::FILE* file = static_cast<std::FILE*>(userdata);
    return file && std::fwrite(block, 1, length, file) == length;
}

std::size_t readFromFile(char* buffer, std::size_t max_length, void* userdata) {
    std::FILE* file = static_cast<std::FILE*>(userdata);
    return file ? std::fread(buffer, 1, max_length, file) : 0;
}

circuit::Pin* findPin(const std::string& name) {
    ensureDefaultObjects();
    if (!g_current_circuit) return nullptr;
    for (std::size_t i = 0; i < g_current_circuit->pinCount(); ++i) {
        circuit::Pin* pin = g_current_circuit->pinByIndex(i);
        if (pin && name == pin->name) return pin;
    }
    return nullptr;
}

void ensureDefaultObjects() {
    if (!g_current_model) {
        g_current_model = g_allocator.allocate<math::MathModel>(static_cast<void*>(0), core::ObjectType::METADATA);
        if (g_current_model) {
            new (g_current_model) math::MathModel(&g_allocator);
            g_current_model->initialize(1, "default");
            g_modelRegistry.add(1, "default");
        }
    }
    if (!g_current_circuit) {
        g_current_circuit = g_allocator.allocate<circuit::Circuit>(static_cast<void*>(0), core::ObjectType::METADATA);
        if (g_current_circuit) {
            new (g_current_circuit) circuit::Circuit(&g_allocator);
            g_current_circuit->initialize(1, "default");
            g_circuitRegistry.add(1, "default");
        }
    }
}

// ============================================================
// TAB COMPLETION
// ============================================================
std::vector<std::string> g_completion_matches;

char* command_generator(const char* text, int state) {
    static size_t list_index, len;
    const char* commands[] = {
        "init", "model", "circuit", "var", "variable", "const", "constant",
        "equation", "eq", "inequality", "ineq", "function", "func",
        "compile", "pin", "lock", "unlock", "solve", "list", "stats",
        "allocator", "prng", "save", "load", "help", "quit", "exit",
        "pinmatrix", "subcircuit", "instance", "bus", "gate",
        "md5", "sha256", "sha512", "sha3", "aes", "modexp", "eccadd", "eccmul", "ecdsa",
        nullptr
    };

    if (!state) { list_index = 0; len = std::strlen(text); }
    while (commands[list_index]) {
        if (std::strncmp(commands[list_index], text, len) == 0) {
            return strdup(commands[list_index++]);
        }
        list_index++;
    }
    return nullptr;
}

char** semon_completion(const char* text, int start, int end) {
    (void)start; (void)end;
    return rl_completion_matches(text, command_generator);
}

void printCompletions(const std::string& prefix) {
    const char* commands[] = {
        "init", "model", "circuit", "var", "variable", "const", "constant",
        "equation", "eq", "inequality", "ineq", "function", "func",
        "compile", "pin", "lock", "unlock", "solve", "list", "stats",
        "allocator", "prng", "save", "load", "help", "quit", "exit",
        "pinmatrix", "subcircuit", "instance", "bus", "gate",
        "md5", "sha256", "sha512", "sha3", "aes", "modexp", "eccadd", "eccmul", "ecdsa",
        nullptr
    };
    bool printed = false;
    for (size_t i = 0; commands[i]; ++i) {
        std::string cmd(commands[i]);
        if (cmd.compare(0, prefix.size(), prefix) == 0) {
            std::cout << cmd << " ";
            printed = true;
        }
    }
    if (printed) std::cout << "\n";
}

// ============================================================
// PIN GRID MATRIX
// ============================================================
bool commandPinMatrix() {
    ensureDefaultObjects();
    if (!g_current_circuit) {
        std::cout << "no circuit\n";
        return false;
    }

    std::cout << "Pin Grid Matrix (" << g_current_circuit->pinCount() << " pins):\n";
    std::cout << "+----+----------------------+-------+--------+--------+----------+--------+\n";
    std::cout << "| #  | Name                 | UID   | State  | Locked | Var:Bit  | Bus    |\n";
    std::cout << "+----+----------------------+-------+--------+--------+----------+--------+\n";

    for (size_t i = 0; i < g_current_circuit->pinCount(); ++i) {
        circuit::Pin* pin = g_current_circuit->pinByIndex(i);
        if (!pin) continue;

        std::cout << "| " << std::setw(2) << i << " | ";
        std::cout << std::left << std::setw(20) << pin->name << " | ";
        std::cout << std::setw(5) << pin->uid << " | ";
        std::cout << stateColor(pin->state) << std::setw(6) << stateName(pin->state);
        resetColor();
        std::cout << " | ";
        std::cout << std::setw(6) << (pin->locked ? "yes" : "no") << " | ";
        if (pin->variable_uid) {
            std::cout << std::setw(4) << pin->variable_uid << ":" << std::setw(4) << pin->bit_index << " | ";
        } else {
            std::cout << "  -   | ";
        }
        std::cout << std::setw(6) << (pin->bus ? "yes" : "no") << " |\n";
    }
    std::cout << "+----+----------------------+-------+--------+--------+----------+--------+\n";

    // Gate matrix
    std::cout << "\nGate Matrix (" << g_current_circuit->gateCount() << " gates):\n";
    std::cout << "+----+--------+--------+--------+--------+----------+\n";
    std::cout << "| #  | UID    | Truth  | Pin A  | Pin B  | Pin C    |\n";
    std::cout << "+----+--------+--------+--------+--------+----------+\n";

    for (size_t i = 0; i < g_current_circuit->gateCount(); ++i) {
        circuit::Virtual3PinGate* gate = g_current_circuit->gateByIndex(i);
        if (!gate || gate->isReleased()) continue;
        std::cout << "| " << std::setw(2) << i << " | ";
        std::cout << std::setw(6) << gate->uid() << " | ";
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)gate->truthByte() << std::dec << std::setfill(' ') << " | ";
        std::cout << std::setw(6) << (gate->pinA() ? gate->pinA()->uid : 0) << " | ";
        std::cout << std::setw(6) << (gate->pinB() ? gate->pinB()->uid : 0) << " | ";
        std::cout << std::setw(8) << (gate->pinC() ? gate->pinC()->uid : 0) << " |\n";
    }
    std::cout << "+----+--------+--------+--------+--------+----------+\n";

    return true;
}

// ============================================================
// COMMAND IMPLEMENTATIONS
// ============================================================
bool commandInit() {
    ensureDefaultObjects();
    std::cout << "default model and circuit ready\n";
    return true;
}

bool commandModel(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 2) { std::cout << "usage: model <name>\n"; return false; }
    std::uint32_t uid = static_cast<std::uint32_t>(g_modelRegistry.count() + 1);
    math::MathModel* model = g_allocator.allocate<math::MathModel>(static_cast<void*>(0), core::ObjectType::METADATA);
    if (!model) return false;
    new (model) math::MathModel(&g_allocator);
    if (!model->initialize(uid, args[1].c_str())) return false;
    g_current_model = model;
    g_modelRegistry.add(uid, args[1].c_str());
    std::cout << "model " << args[1] << " uid=" << uid << "\n";
    return true;
}

bool commandCircuit(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 2) { std::cout << "usage: circuit <name>\n"; return false; }
    std::uint32_t uid = static_cast<std::uint32_t>(g_circuitRegistry.count() + 1);
    circuit::Circuit* c = g_allocator.allocate<circuit::Circuit>(static_cast<void*>(0), core::ObjectType::METADATA);
    if (!c) return false;
    new (c) circuit::Circuit(&g_allocator);
    if (!c->initialize(uid, args[1].c_str())) return false;
    g_current_circuit = c;
    g_circuitRegistry.add(uid, args[1].c_str());
    std::cout << "circuit " << args[1] << " uid=" << uid << "\n";
    return true;
}

bool commandVariable(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 2) { std::cout << "usage: var <name> [bits]\n"; return false; }
    std::uint32_t bits = 1;
    if (args.size() >= 3) { std::istringstream(args[2]) >> bits; }
    std::uint32_t uid = 0;
    if (!g_current_model->addVariable(args[1].c_str(), bits, &uid)) {
        std::cout << "failed to add variable\n"; return false;
    }
    std::cout << "variable " << args[1] << " uid=" << uid << " bits=" << bits << "\n";
    return true;
}

bool commandConstant(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 3) { std::cout << "usage: const <name> <value> [bits]\n"; return false; }
    std::uint64_t value = 0; std::istringstream(args[2]) >> value;
    std::uint32_t bits = 64;
    if (args.size() >= 4) { std::istringstream(args[3]) >> bits; }
    std::uint32_t uid = 0;
    if (!g_current_model->addConstant(args[1].c_str(), value, bits, &uid)) {
        std::cout << "failed to add constant\n"; return false;
    }
    std::cout << "constant " << args[1] << " uid=" << uid << " value=" << value << " bits=" << bits << "\n";
    return true;
}

bool commandEquation(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 2) { std::cout << "usage: equation <text>\n"; return false; }
    std::string text;
    for (size_t i = 1; i < args.size(); ++i) { if (i > 1) text += " "; text += args[i]; }
    char error[128]; std::uint32_t uid = 0;
    if (!g_current_model->parseAndAddEquation(text.c_str(), &uid, error, sizeof(error))) {
        std::cout << "equation parse failed: " << error << "\n"; return false;
    }
    std::cout << "equation uid=" << uid << "\n";
    return true;
}

bool commandInequality(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 4) { std::cout << "usage: ineq <name> <lhs> <op> <rhs>\n"; return false; }
    // Parse: ineq name A < B
    std::string lhs = args[2];
    std::string op = args[3];
    std::string rhs = args[4];

    math::MathOperation rel = math::MathOperation::NONE;
    if (op == "<") rel = math::MathOperation::LT;
    else if (op == ">") rel = math::MathOperation::GT;
    else if (op == "<=") rel = math::MathOperation::LE;
    else if (op == ">=") rel = math::MathOperation::GE;
    else if (op == "!=") rel = math::MathOperation::NE;
    else { std::cout << "invalid operator\n"; return false; }

    math::ExpressionParser parser(*g_current_model);
    math::ExpressionNode* lhs_node = nullptr;
    math::ExpressionNode* rhs_node = nullptr;
    char error[128];
    if (!parser.parse(lhs.c_str(), &lhs_node, error, sizeof(error)) ||
        !parser.parse(rhs.c_str(), &rhs_node, error, sizeof(error))) {
        std::cout << "parse failed: " << error << "\n"; return false;
    }

    std::uint32_t uid = 0;
    if (!g_current_model->addInequality(args[1].c_str(), lhs_node, rhs_node, rel, &uid)) {
        std::cout << "add inequality failed\n"; return false;
    }
    std::cout << "inequality uid=" << uid << "\n";
    return true;
}

bool commandFunction(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 3) { std::cout << "usage: func <name> <arg1> [arg2...] -- <body>\n"; return false; }

    std::vector<std::string> arg_names;
    size_t body_start = 0;
    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i] == "--") { body_start = i + 1; break; }
        arg_names.push_back(args[i]);
    }
    if (body_start == 0) { std::cout << "missing -- separator\n"; return false; }

    std::string body_text;
    for (size_t i = body_start; i < args.size(); ++i) {
        if (i > body_start) body_text += " ";
        body_text += args[i];
    }

    math::ExpressionParser parser(*g_current_model);
    math::ExpressionNode* body_node = nullptr;
    char error[128];
    if (!parser.parse(body_text.c_str(), &body_node, error, sizeof(error))) {
        std::cout << "body parse failed: " << error << "\n"; return false;
    }

    std::uint32_t uid = 0;
    const char** arg_ptrs = new const char*[arg_names.size()];
    for (size_t i = 0; i < arg_names.size(); ++i) arg_ptrs[i] = arg_names[i].c_str();

    if (!g_current_model->addFunction(args[1].c_str(), arg_ptrs, static_cast<std::uint32_t>(arg_names.size()), body_node, &uid)) {
        std::cout << "add function failed\n"; delete[] arg_ptrs; return false;
    }
    delete[] arg_ptrs;
    std::cout << "function " << args[1] << " uid=" << uid << " args=" << arg_names.size() << "\n";
    return true;
}

bool commandCompile() {
    ensureDefaultObjects();
    if (!g_current_model || !g_current_circuit) { std::cout << "model or circuit missing\n"; return false; }
    if (!math::Synthesizer::compileModelToCircuit(*g_current_model, *g_current_circuit)) {
        std::cout << "compile failed\n"; return false;
    }
    std::cout << "compiled " << g_current_model->variableCount() << " variables and "
              << g_current_model->equationCount() << " equations\n";
    return true;
}

bool commandCrypto(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 2) { std::cout << "usage: <crypto> <input_var> [output_prefix]\n"; return false; }

    const char* input = args[1].c_str();
    const char* output = args.size() >= 3 ? args[2].c_str() : "out";

    bool ok = false;
    if (args[0] == "md5") ok = math::CryptoBuiltins::buildMD5(*g_current_model, input, output, *g_current_circuit, &g_allocator);
    else if (args[0] == "sha256") ok = math::CryptoBuiltins::buildSHA256(*g_current_model, input, output, *g_current_circuit, &g_allocator);
    else if (args[0] == "sha512") ok = math::CryptoBuiltins::buildSHA512(*g_current_model, input, output, *g_current_circuit, &g_allocator);
    else if (args[0] == "sha3") ok = math::CryptoBuiltins::buildSHA3(*g_current_model, input, output, *g_current_circuit, &g_allocator);
    else if (args[0] == "aes") {
        if (args.size() < 4) { std::cout << "usage: aes <key_var> <data_var> [key_bits=128]\n"; return false; }
        std::uint32_t key_bits = 128;
        if (args.size() >= 5) std::istringstream(args[4]) >> key_bits;
        ok = math::CryptoBuiltins::buildAES(*g_current_model, args[1].c_str(), args[2].c_str(), output, key_bits, *g_current_circuit, &g_allocator);
    }
    else if (args[0] == "modexp") {
        if (args.size() < 5) { std::cout << "usage: modexp <base> <exp> <mod> <output>\n"; return false; }
        ok = math::CryptoBuiltins::buildMODEXP(*g_current_model, args[1].c_str(), args[2].c_str(), args[3].c_str(), args[4].c_str(), *g_current_circuit, &g_allocator);
    }
    else if (args[0] == "eccadd") {
        if (args.size() < 5) { std::cout << "usage: eccadd <p1> <p2> <curve> <output>\n"; return false; }
        ok = math::CryptoBuiltins::buildECCAdd(*g_current_model, args[1].c_str(), args[2].c_str(), args[3].c_str(), args[4].c_str(), *g_current_circuit, &g_allocator);
    }
    else if (args[0] == "eccmul") {
        if (args.size() < 5) { std::cout << "usage: eccmul <scalar> <point> <curve> <output>\n"; return false; }
        ok = math::CryptoBuiltins::buildECCMul(*g_current_model, args[1].c_str(), args[2].c_str(), args[3].c_str(), args[4].c_str(), *g_current_circuit, &g_allocator);
    }
    else if (args[0] == "ecdsa") {
        if (args.size() < 6) { std::cout << "usage: ecdsa <hash> <sig> <pubkey> <curve> <output>\n"; return false; }
        ok = math::CryptoBuiltins::buildECDSAVerify(*g_current_model, args[1].c_str(), args[2].c_str(), args[3].c_str(), args[4].c_str(), args[5].c_str(), *g_current_circuit, &g_allocator);
    }

    if (!ok) std::cout << "crypto build failed\n";
    return ok;
}

bool commandPin(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 2) { std::cout << "usage: pin <name> [variable-uid bit]\n"; return false; }
    std::uint32_t var_uid = 0, bit = 0;
    if (args.size() >= 4) { std::istringstream(args[2]) >> var_uid; std::istringstream(args[3]) >> bit; }
    circuit::Pin* pin = g_current_circuit->addPin(args[1].c_str(), var_uid, bit);
    if (!pin) { std::cout << "pin allocation failed\n"; return false; }
    std::cout << "pin " << args[1] << " uid=" << pin->uid << "\n";
    return true;
}

bool commandLock(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 3) { std::cout << "usage: lock <pin-name> <0|1>\n"; return false; }
    circuit::Pin* pin = findPin(args[1]);
    if (!pin) { std::cout << "pin not found\n"; return false; }
    bool value = args[2] == "1";
    if (!g_current_circuit->lockPin(pin, value)) return false;
    std::cout << "locked " << args[1] << " to " << args[2] << "\n";
    return true;
}

bool commandUnlock(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 2) { std::cout << "usage: unlock <pin-name>\n"; return false; }
    circuit::Pin* pin = findPin(args[1]);
    if (!pin) { std::cout << "pin not found\n"; return false; }
    g_current_circuit->unlockPin(pin);
    std::cout << "unlocked " << args[1] << "\n";
    return true;
}

bool commandSolve() {
    ensureDefaultObjects();
    if (!g_current_circuit) { std::cout << "circuit missing\n"; return false; }
    bool ok = circuit::Solver::propagate(*g_current_circuit);
    std::cout << (ok ? "propagation complete\n" : "propagation stopped\n");
    return ok;
}

bool commandList() {
    ensureDefaultObjects();
    std::cout << "models:\n";
    for (size_t i = 0; i < g_modelRegistry.count(); ++i) {
        const core::RegistryItem* item = g_modelRegistry.at(i);
        if (item && item->active) std::cout << "  " << item->uid << " " << item->name << "\n";
    }
    std::cout << "circuits:\n";
    for (size_t i = 0; i < g_circuitRegistry.count(); ++i) {
        const core::RegistryItem* item = g_circuitRegistry.at(i);
        if (item && item->active) std::cout << "  " << item->uid << " " << item->name << "\n";
    }
    if (g_current_circuit) {
        std::cout << "pins:\n";
        for (size_t i = 0; i < g_current_circuit->pinCount(); ++i) {
            circuit::Pin* pin = g_current_circuit->pinByIndex(i);
            if (pin) {
                std::cout << "  " << pin->uid << " " << pin->name << " ";
                std::cout << stateColor(pin->state) << stateName(pin->state);
                resetColor();
                if (pin->locked) std::cout << " locked";
                std::cout << "\n";
            }
        }
        std::cout << "gates:\n";
        for (size_t i = 0; i < g_current_circuit->gateCount(); ++i) {
            circuit::Virtual3PinGate* gate = g_current_circuit->gateByIndex(i);
            if (gate && !gate->isReleased()) {
                std::cout << "  " << gate->uid() << " truth=0x" << std::hex << (int)gate->truthByte() << std::dec << "\n";
            }
        }
        std::cout << "buses: " << g_current_circuit->busCount() << "\n";
        std::cout << "subcircuits: " << g_current_circuit->subcircuitCount() << "\n";
    }
    return true;
}

bool commandStats() {
    ensureDefaultObjects();
    std::cout << "allocator pages total=" << g_allocator.pageCount()
              << " used=" << g_allocator.usedPageCount()
              << " free=" << g_allocator.freePageCount()
              << " active_objects=" << g_allocator.activeObjectCount()
              << " released_objects=" << g_allocator.releasedObjectCount()
              << " chains=" << g_allocator.chainCount() << "\n";
    std::cout << "diagnostics error=" << (diagnostics::g_errorFlag ? "true" : "false")
              << " cycle=" << diagnostics::Diagnostics::cycleCount() << "\n";
    std::cout << "prng seed=" << g_prng.seedValue() << " counter=" << g_prng.counter() << "\n";
    return true;
}

bool commandAllocator() {
    ensureDefaultObjects();
    for (size_t ci = 0; ci < g_allocator.chainCount(); ++ci) {
        const core::PageChain* chain = g_allocator.chainAt(ci);
        if (!chain) continue;
        std::cout << "chain parent=" << chain->parent << " type=" << (int)chain->type
                  << " pages=" << chain->page_count << " active=" << chain->active_objects
                  << " free_slots=" << chain->free_objects << "\n";
        for (const core::PageHeader* page = chain->head; page; page = page->next) {
            std::cout << "  page=" << page << " active=" << page->active_count
                      << " dead=" << page->dead_count << " unused=" << page->bytes_unused << "\n";
        }
    }
    return true;
}

bool commandPrng(const std::vector<std::string>& args) {
    if (args.size() < 2) { std::cout << "usage: prng <seed>\n"; return false; }
    std::uint64_t seed = 0; std::istringstream(args[1]) >> seed;
    g_prng.seed(seed);
    std::cout << "prng seeded\n";
    return true;
}

bool commandSave(const std::vector<std::string>& args) {
    if (args.size() < 2) { std::cout << "usage: save <path>\n"; return false; }
    std::FILE* file = std::fopen(args[1].c_str(), "wb");
    if (!file) return false;
    persistence::JsonWriter writer(writeToFile, file);
    bool ok = persistence::saveSession(*g_current_model, *g_current_circuit, g_prng, writer);
    std::fclose(file);
    return ok && writer.ok();
}

bool commandLoad(const std::vector<std::string>& args) {
    if (args.size() < 2) { std::cout << "usage: load <path>\n"; return false; }
    std::FILE* file = std::fopen(args[1].c_str(), "rb");
    if (!file) return false;
    persistence::JsonReader reader(readFromFile, file);
    bool ok = reader.consume('{') && reader.ok();
    if (ok) ok = persistence::loadSession(*g_current_model, *g_current_circuit, g_prng, reader);
    std::fclose(file);
    return ok && reader.ok();
}

bool commandSubcircuit(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 3) { std::cout << "usage: instance <name> <blueprint-circuit>\n"; return false; }
    circuit::Circuit* blueprint = nullptr;
    for (size_t i = 0; i < g_circuitRegistry.count(); ++i) {
        const core::RegistryItem* item = g_circuitRegistry.at(i);
        if (item && item->active && std::strcmp(item->name, args[2].c_str()) == 0) {
            // Find circuit by uid - would need registry lookup
        }
    }
    std::cout << "subcircuit instantiation not fully wired\n";
    return false;
}

bool commandBus(const std::vector<std::string>& args) {
    ensureDefaultObjects();
    if (args.size() < 2) { std::cout << "usage: bus <name>\n"; return false; }
    circuit::Bus* bus = g_current_circuit->addBus(args[1].c_str());
    if (!bus) { std::cout << "bus allocation failed\n"; return false; }
    std::cout << "bus " << args[1] << " uid=" << bus->uid() << "\n";
    return true;
}

void printHelp() {
    std::cout << "semon commands:\n"
              << "  init                         create default model and circuit\n"
              << "  model <name>                 create/select mathematical model\n"
              << "  circuit <name>               create/select circuit topology\n"
              << "  var <name> [bits]            add variable, default 1 bit\n"
              << "  const <name> <value> [bits]  add constant\n"
              << "  equation <text>              parse and add equation, e.g. A == B\n"
              << "  ineq <name> <lhs> <op> <rhs> add inequality (op: < > <= >= !=)\n"
              << "  func <name> <args> -- <body> add user function\n"
              << "  compile                      synthesize active model into active circuit\n"
              << "  pin <name> [var bit]         add direction-agnostic pin\n"
              << "  bus <name>                   add signal bus\n"
              << "  instance <name> <blueprint>  instantiate subcircuit\n"
              << "  lock <pin-name> <0|1>        lock a pin value\n"
              << "  unlock <pin-name>            release a pin lock\n"
              << "  solve                        run bidirectional propagation\n"
              << "  list                         list models, circuits, pins, gates\n"
              << "  pinmatrix                    show pin/gate grid matrix\n"
              << "  stats                        show allocator, solver, PRNG state\n"
              << "  allocator                    show page-chain telemetry\n"
              << "  prng <seed>                  reseed deterministic PRNG\n"
              << "  save <path> / load <path>    stream JSON persistence\n"
              << "  md5 <input> [out]            build MD5 constraints\n"
              << "  sha256 <input> [out]         build SHA-256 constraints\n"
              << "  sha512 <input> [out]         build SHA-512 constraints\n"
              << "  sha3 <input> [out]           build SHA-3/Keccak constraints\n"
              << "  aes <key> <data> [bits]      build AES constraints\n"
              << "  modexp <base> <exp> <mod> <out> build MODEXP constraints\n"
              << "  eccadd <p1> <p2> <curve> <out> build ECC point addition\n"
              << "  eccmul <scalar> <pt> <curve> <out> build ECC scalar mul\n"
              << "  ecdsa <hash> <sig> <key> <curve> <out> build ECDSA verify\n"
              << "  help / quit\n";
}

// ============================================================
// SHELL IMPLEMENTATION
// ============================================================
bool executeSingleCommand(const std::vector<std::string>& args) {
    if (args.empty()) return true;
    const std::string& cmd = args[0];

    if (cmd == "help") { printHelp(); return true; }
    if (cmd == "quit" || cmd == "exit") return false;
    if (cmd == "init") return commandInit();
    if (cmd == "model") return commandModel(args);
    if (cmd == "circuit") return commandCircuit(args);
    if (cmd == "var" || cmd == "variable") return commandVariable(args);
    if (cmd == "const" || cmd == "constant") return commandConstant(args);
    if (cmd == "equation" || cmd == "eq") return commandEquation(args);
    if (cmd == "inequality" || cmd == "ineq") return commandInequality(args);
    if (cmd == "function" || cmd == "func") return commandFunction(args);
    if (cmd == "compile") return commandCompile();
    if (cmd == "pin") return commandPin(args);
    if (cmd == "bus") return commandBus(args);
    if (cmd == "instance" || cmd == "subcircuit") return commandSubcircuit(args);
    if (cmd == "lock") return commandLock(args);
    if (cmd == "unlock") return commandUnlock(args);
    if (cmd == "solve") return commandSolve();
    if (cmd == "list") return commandList();
    if (cmd == "pinmatrix") return commandPinMatrix();
    if (cmd == "stats") return commandStats();
    if (cmd == "allocator") return commandAllocator();
    if (cmd == "prng") return commandPrng(args);
    if (cmd == "save") return commandSave(args);
    if (cmd == "load") return commandLoad(args);
    if (cmd == "md5" || cmd == "sha256" || cmd == "sha512" || cmd == "sha3" ||
        cmd == "aes" || cmd == "modexp" || cmd == "eccadd" || cmd == "eccmul" || cmd == "ecdsa")
        return commandCrypto(args);

    std::cout << "unknown command: " << cmd << "\n";
    return true;
}

bool executeLine(const std::string& line) {
    size_t start = 0;
    while (start <= line.size()) {
        size_t end = line.find(';', start);
        std::string segment = line.substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::istringstream stream(segment);
        std::string command; stream >> command;
        if (!command.empty()) {
            std::vector<std::string> args{command};
            std::string token;
            while (stream >> token) args.push_back(token);
            if (!executeSingleCommand(args)) return false;
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return true;
}

void loadHistory(std::vector<std::string>& history) {
    const char* home = std::getenv("HOME");
    if (!home) return;
    std::string path = std::string(home) + "/.semon_history";
    std::ifstream input(path.c_str());
    std::string line;
    while (std::getline(input, line)) {
        history.push_back(line);
        add_history(line.c_str());
    }
}

void saveHistory(const std::vector<std::string>& history) {
    const char* home = std::getenv("HOME");
    if (!home) return;
    std::string path = std::string(home) + "/.semon_history";
    std::ofstream output(path.c_str(), std::ios::out | std::ios::trunc);
    for (const auto& line : history) output << line << "\n";
}

} // anonymous namespace

Shell::Shell() : running_(true) {
    g_allocator.initialize(64);
    g_prng.seed(1);
    ensureDefaultObjects();
    rl_attempted_completion_function = semon_completion;
}

int Shell::run(int argc, char** argv) {
    if (argc > 1) {
        std::string cmd;
        for (int i = 1; i < argc; ++i) { if (i > 1) cmd += " "; cmd += argv[i]; }
        return executeLine(cmd) ? 0 : 1;
    }

    std::vector<std::string> history;
    loadHistory(history);
    printHelp();

    while (running_) {
        char* line = readline("semon> ");
        if (!line) break;
        std::string line_str(line);
        free(line);

        if (line_str.empty()) continue;
        if (line_str == "\t") { printCompletions(""); continue; }

        bool keep = executeLine(line_str);
        if (!keep) break;
        history.push_back(line_str);
        add_history(line_str.c_str());
        saveHistory(history);
    }
    saveHistory(history);
    return 0;
}

} // namespace core
} // namespace semon
