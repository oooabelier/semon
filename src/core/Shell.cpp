#include "core/Shell.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <vector>

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
#include "persistence/JsonStreaming.hpp"

namespace semon {
namespace core {

namespace {
PagedAllocator g_allocator;
DeterministicPrng g_prng;
math::MathModel* g_current_model = static_cast<math::MathModel*>(0);
circuit::Circuit* g_current_circuit = static_cast<circuit::Circuit*>(0);

const char* stateColor(circuit::SignalState state)
{
    switch (state) {
    case circuit::SignalState::ONE:
        return "\033[32m";
    case circuit::SignalState::ZERO:
        return "\033[31m";
    case circuit::SignalState::CONFLICT:
        return "\033[35m";
    case circuit::SignalState::FLOATING:
    default:
        return "\033[33m";
    }
}

const char* stateName(circuit::SignalState state)
{
    switch (state) {
    case circuit::SignalState::ONE:
        return "1";
    case circuit::SignalState::ZERO:
        return "0";
    case circuit::SignalState::CONFLICT:
        return "!";
    case circuit::SignalState::FLOATING:
    default:
        return "?";
    }
}

void resetColor()
{
    std::cout << "\033[0m";
}

bool writeToFile(const char* block, std::size_t length, void* userdata)
{
    std::FILE* file = static_cast<std::FILE*>(userdata);
    return file != static_cast<std::FILE*>(0) && std::fwrite(block, 1U, length, file) == length;
}

std::size_t readFromFile(char* buffer, std::size_t max_length, void* userdata)
{
    std::FILE* file = static_cast<std::FILE*>(userdata);
    if (file == static_cast<std::FILE*>(0)) {
        return 0U;
    }
    return std::fread(buffer, 1U, max_length, file);
}

void printHelp()
{
    std::cout << "semon commands:\n"
              << "  init                         create default model and circuit\n"
              << "  model <name>                 create/select mathematical model\n"
              << "  circuit <name>               create/select circuit topology\n"
              << "  var <name> [bits]            add variable, default 1 bit\n"
              << "  equation <text>              parse and add equation, e.g. A == B\n"
              << "  compile                      synthesize active model into active circuit\n"
              << "  pin <name> [var bit]         add direction-agnostic pin\n"
              << "  lock <pin-name> <0|1>        lock a pin value\n"
              << "  unlock <pin-name>            release a pin lock\n"
              << "  solve                        run bidirectional propagation\n"
              << "  list                         list models, circuits, pins, gates\n"
              << "  stats                        show allocator, solver, and PRNG state\n"
              << "  allocator                    show page-chain telemetry\n"
              << "  prng <seed>                  reseed deterministic PRNG\n"
              << "  save <path> / load <path>    stream JSON persistence\n"
              << "  help / quit\n";
}

void ensureDefaultObjects()
{
    if (g_current_model == static_cast<math::MathModel*>(0)) {
        g_current_model = g_allocator.allocate<math::MathModel>(static_cast<void*>(0), core::ObjectType::METADATA);
        if (g_current_model != static_cast<math::MathModel*>(0)) {
            new (g_current_model) math::MathModel(&g_allocator);
            (void)g_current_model->initialize(1U, "default");
            (void)g_modelRegistry.add(1U, "default");
        }
    }

    if (g_current_circuit == static_cast<circuit::Circuit*>(0)) {
        g_current_circuit = g_allocator.allocate<circuit::Circuit>(static_cast<void*>(0), core::ObjectType::METADATA);
        if (g_current_circuit != static_cast<circuit::Circuit*>(0)) {
            new (g_current_circuit) circuit::Circuit(&g_allocator);
            (void)g_current_circuit->initialize(1U, "default");
            (void)g_circuitRegistry.add(1U, "default");
        }
    }
}

circuit::Pin* findPin(const std::string& name)
{
    ensureDefaultObjects();
    if (g_current_circuit == static_cast<circuit::Circuit*>(0)) {
        return static_cast<circuit::Pin*>(0);
    }

    for (std::size_t index = 0U; index < g_current_circuit->pinCount(); ++index) {
        circuit::Pin* pin = g_current_circuit->pinByIndex(index);
        if (pin != static_cast<circuit::Pin*>(0) && name == pin->name) {
            return pin;
        }
    }
    return static_cast<circuit::Pin*>(0);
}

bool commandInit()
{
    ensureDefaultObjects();
    std::cout << "default model and circuit ready\n";
    return true;
}

bool commandModel(const std::vector<std::string>& args)
{
    ensureDefaultObjects();
    if (args.size() < 2U) {
        std::cout << "usage: model <name>\n";
        return false;
    }

    const std::uint32_t uid = static_cast<std::uint32_t>(g_modelRegistry.count() + 1U);
    math::MathModel* model = g_allocator.allocate<math::MathModel>(static_cast<void*>(0), core::ObjectType::METADATA);
    if (model == static_cast<math::MathModel*>(0)) {
        return false;
    }
    new (model) math::MathModel(&g_allocator);
    if (!model->initialize(uid, args[1].c_str())) {
        return false;
    }
    g_current_model = model;
    (void)g_modelRegistry.add(uid, args[1].c_str());
    std::cout << "model " << args[1] << " uid=" << uid << "\n";
    return true;
}

bool commandCircuit(const std::vector<std::string>& args)
{
    ensureDefaultObjects();
    if (args.size() < 2U) {
        std::cout << "usage: circuit <name>\n";
        return false;
    }

    const std::uint32_t uid = static_cast<std::uint32_t>(g_circuitRegistry.count() + 1U);
    circuit::Circuit* circuit = g_allocator.allocate<circuit::Circuit>(static_cast<void*>(0), core::ObjectType::METADATA);
    if (circuit == static_cast<circuit::Circuit*>(0)) {
        return false;
    }
    new (circuit) circuit::Circuit(&g_allocator);
    if (!circuit->initialize(uid, args[1].c_str())) {
        return false;
    }
    g_current_circuit = circuit;
    (void)g_circuitRegistry.add(uid, args[1].c_str());
    std::cout << "circuit " << args[1] << " uid=" << uid << "\n";
    return true;
}

bool commandVariable(const std::vector<std::string>& args)
{
    ensureDefaultObjects();
    if (args.size() < 2U) {
        std::cout << "usage: var <name> [bits]\n";
        return false;
    }

    std::uint32_t bits = 1U;
    if (args.size() >= 3U) {
        std::istringstream bits_stream(args[2]);
        bits_stream >> bits;
    }

    std::uint32_t uid = 0U;
    if (!g_current_model->addVariable(args[1].c_str(), bits, &uid)) {
        std::cout << "failed to add variable\n";
        return false;
    }
    std::cout << "variable " << args[1] << " uid=" << uid << " bits=" << bits << "\n";
    return true;
}

bool commandEquation(const std::vector<std::string>& args)
{
    ensureDefaultObjects();
    if (args.size() < 2U) {
        std::cout << "usage: equation <text>\n";
        return false;
    }

    std::string text;
    for (std::size_t index = 1U; index < args.size(); ++index) {
        if (index > 1U) {
            text += " ";
        }
        text += args[index];
    }

    char error[128];
    std::uint32_t uid = 0U;
    if (!g_current_model->parseAndAddEquation(text.c_str(), &uid, error, sizeof(error))) {
        std::cout << "equation parse failed: " << error << "\n";
        return false;
    }
    std::cout << "equation uid=" << uid << "\n";
    return true;
}

bool commandCompile()
{
    ensureDefaultObjects();
    if (g_current_model == static_cast<math::MathModel*>(0) || g_current_circuit == static_cast<circuit::Circuit*>(0)) {
        std::cout << "model or circuit missing\n";
        return false;
    }

    if (!semon::math::Synthesizer::compileModelToCircuit(*g_current_model, *g_current_circuit)) {
        std::cout << "compile failed\n";
        return false;
    }
    std::cout << "compiled " << g_current_model->variableCount() << " variables and "
              << g_current_model->equationCount() << " equations\n";
    return true;
}

bool commandPin(const std::vector<std::string>& args)
{
    ensureDefaultObjects();
    if (args.size() < 2U) {
        std::cout << "usage: pin <name> [variable-uid bit]\n";
        return false;
    }

    std::uint32_t variable_uid = 0U;
    std::uint32_t bit = 0U;
    if (args.size() >= 4U) {
        std::istringstream variable_stream(args[2]);
        std::istringstream bit_stream(args[3]);
        variable_stream >> variable_uid;
        bit_stream >> bit;
    }

    circuit::Pin* pin = g_current_circuit->addPin(args[1].c_str(), variable_uid, bit);
    if (pin == static_cast<circuit::Pin*>(0)) {
        std::cout << "pin allocation failed\n";
        return false;
    }
    std::cout << "pin " << args[1] << " uid=" << pin->uid << "\n";
    return true;
}

bool commandLock(const std::vector<std::string>& args)
{
    ensureDefaultObjects();
    if (args.size() < 3U) {
        std::cout << "usage: lock <pin-name> <0|1>\n";
        return false;
    }

    circuit::Pin* pin = findPin(args[1]);
    if (pin == static_cast<circuit::Pin*>(0)) {
        std::cout << "pin not found\n";
        return false;
    }

    const bool value = args[2] == "1";
    if (!g_current_circuit->lockPin(pin, value)) {
        return false;
    }
    std::cout << "locked " << args[1] << " to " << args[2] << "\n";
    return true;
}

bool commandUnlock(const std::vector<std::string>& args)
{
    ensureDefaultObjects();
    if (args.size() < 2U) {
        std::cout << "usage: unlock <pin-name>\n";
        return false;
    }

    circuit::Pin* pin = findPin(args[1]);
    if (pin == static_cast<circuit::Pin*>(0)) {
        std::cout << "pin not found\n";
        return false;
    }
    (void)g_current_circuit->unlockPin(pin);
    std::cout << "unlocked " << args[1] << "\n";
    return true;
}

bool commandSolve()
{
    ensureDefaultObjects();
    if (g_current_circuit == static_cast<circuit::Circuit*>(0)) {
        std::cout << "circuit missing\n";
        return false;
    }

    const bool ok = circuit::Solver::propagate(*g_current_circuit);
    std::cout << (ok ? "propagation complete\n" : "propagation stopped\n");
    return ok;
}

bool commandList()
{
    ensureDefaultObjects();
    std::cout << "models:\n";
    for (std::size_t index = 0U; index < g_modelRegistry.count(); ++index) {
        const core::RegistryItem* item = g_modelRegistry.at(index);
        if (item != static_cast<const core::RegistryItem*>(0) && item->active) {
            std::cout << "  " << item->uid << " " << item->name << "\n";
        }
    }

    std::cout << "circuits:\n";
    for (std::size_t index = 0U; index < g_circuitRegistry.count(); ++index) {
        const core::RegistryItem* item = g_circuitRegistry.at(index);
        if (item != static_cast<const core::RegistryItem*>(0) && item->active) {
            std::cout << "  " << item->uid << " " << item->name << "\n";
        }
    }

    if (g_current_circuit != static_cast<circuit::Circuit*>(0)) {
        std::cout << "pins:\n";
        for (std::size_t index = 0U; index < g_current_circuit->pinCount(); ++index) {
            circuit::Pin* pin = g_current_circuit->pinByIndex(index);
            if (pin != static_cast<circuit::Pin*>(0)) {
                std::cout << "  " << pin->uid << " " << pin->name << " ";
                std::cout << stateColor(pin->state) << stateName(pin->state);
                resetColor();
                if (pin->locked) {
                    std::cout << " locked";
                }
                std::cout << "\n";
            }
        }

        std::cout << "gates:\n";
        for (std::size_t index = 0U; index < g_current_circuit->gateCount(); ++index) {
            circuit::Virtual3PinGate* gate = g_current_circuit->gateByIndex(index);
            if (gate != static_cast<circuit::Virtual3PinGate*>(0)) {
                std::cout << "  " << gate->uid() << " truth=0x";
                std::cout << std::hex << static_cast<unsigned int>(gate->truthByte()) << std::dec << "\n";
            }
        }
    }
    return true;
}

bool commandStats()
{
    ensureDefaultObjects();
    std::cout << "allocator pages total=" << g_allocator.pageCount()
              << " used=" << g_allocator.usedPageCount()
              << " free=" << g_allocator.freePageCount()
              << " active_objects=" << g_allocator.activeObjectCount()
              << " released_objects=" << g_allocator.releasedObjectCount()
              << " chains=" << g_allocator.chainCount() << "\n";
    std::cout << "diagnostics error=" << (diagnostics::g_errorFlag ? "true" : "false")
              << " cycle=" << diagnostics::Diagnostics::cycleCount() << "\n";
    std::cout << "prng seed=" << g_prng.seedValue()
              << " counter=" << g_prng.counter() << "\n";
    return true;
}

bool commandAllocator()
{
    ensureDefaultObjects();
    for (std::size_t chain_index = 0U; chain_index < g_allocator.chainCount(); ++chain_index) {
        const core::PageChain* chain = g_allocator.chainAt(chain_index);
        if (chain == static_cast<const core::PageChain*>(0)) {
            continue;
        }
        std::cout << "chain parent=" << chain->parent << " type="
                  << static_cast<unsigned int>(chain->type) << " pages=" << chain->page_count
                  << " active=" << chain->active_objects << " free_slots=" << chain->free_objects << "\n";
        for (const core::PageHeader* page = chain->head; page != static_cast<const core::PageHeader*>(0); page = page->next) {
            std::cout << "  page=" << page << " active=" << page->active_count
                      << " dead=" << page->dead_count << " unused=" << page->bytes_unused << "\n";
        }
    }
    return true;
}

bool commandPrng(const std::vector<std::string>& args)
{
    if (args.size() < 2U) {
        std::cout << "usage: prng <seed>\n";
        return false;
    }

    std::istringstream stream(args[1]);
    std::uint64_t seed = 0ULL;
    stream >> seed;
    g_prng.seed(seed);
    std::cout << "prng seeded\n";
    return true;
}

bool commandSave(const std::vector<std::string>& args)
{
    if (args.size() < 2U) {
        std::cout << "usage: save <path>\n";
        return false;
    }

    std::FILE* file = std::fopen(args[1].c_str(), "wb");
    if (file == static_cast<std::FILE*>(0)) {
        return false;
    }

    persistence::JsonWriter writer(writeToFile, file);
    bool ok = writer.writeRaw("{") &&
              writer.writeKey("schema") && writer.writeString("semon-stream-json") &&
              writer.writeRaw(",") &&
              writer.writeKey("models") && writer.writeNumber(g_modelRegistry.count()) &&
              writer.writeRaw(",") &&
              writer.writeKey("circuits") && writer.writeNumber(g_circuitRegistry.count()) &&
              writer.writeRaw(",") &&
              writer.writeKey("prng_seed") && writer.writeNumber(g_prng.seedValue()) &&
              writer.writeRaw("}");
    (void)std::fclose(file);
    return ok && writer.ok();
}

bool commandLoad(const std::vector<std::string>& args)
{
    if (args.size() < 2U) {
        std::cout << "usage: load <path>\n";
        return false;
    }

    std::FILE* file = std::fopen(args[1].c_str(), "rb");
    if (file == static_cast<std::FILE*>(0)) {
        return false;
    }

    persistence::JsonReader reader(readFromFile, file);
    bool ok = reader.consume('{') && reader.ok();
    while (ok && reader.peek() != '\0' && !reader.consume('}')) {
        char key[64];
        ok = reader.readString(key, sizeof(key)) && reader.consume(':');
        if (!ok) {
            break;
        }
        if (std::string(key) == "prng_seed") {
            std::uint64_t seed = 0ULL;
            ok = reader.readUnsigned(&seed);
            if (ok) {
                g_prng.seed(seed);
            }
        } else {
            char discard[64];
            ok = reader.readString(discard, sizeof(discard));
        }
        (void)reader.consume(',');
    }
    (void)std::fclose(file);
    return ok && reader.ok();
}

bool executeSingleCommand(const std::vector<std::string>& args);

bool executeLine(const std::string& line)
{
    std::size_t start = 0U;
    while (start <= line.size()) {
        const std::size_t end = line.find(';', start);
        const std::string segment = line.substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::istringstream stream(segment);
        std::string command;
        stream >> command;
        if (!command.empty()) {
            std::vector<std::string> args;
            args.push_back(command);
            std::string token;
            while (stream >> token) {
                args.push_back(token);
            }

            if (!executeSingleCommand(args)) {
                return false;
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1U;
    }
    return true;
}

bool executeSingleCommand(const std::vector<std::string>& args)
{
    const std::string command = args.empty() ? std::string() : args[0];

    if (command == "help") {
        printHelp();
        return true;
    }
    if (command == "quit" || command == "exit") {
        return false;
    }
    if (command == "init") {
        return commandInit();
    }
    if (command == "model") {
        return commandModel(args);
    }
    if (command == "circuit") {
        return commandCircuit(args);
    }
    if (command == "var") {
        return commandVariable(args);
    }
    if (command == "equation") {
        return commandEquation(args);
    }
    if (command == "compile") {
        return commandCompile();
    }
    if (command == "pin") {
        return commandPin(args);
    }
    if (command == "lock") {
        return commandLock(args);
    }
    if (command == "unlock") {
        return commandUnlock(args);
    }
    if (command == "solve") {
        return commandSolve();
    }
    if (command == "list") {
        return commandList();
    }
    if (command == "stats") {
        return commandStats();
    }
    if (command == "allocator") {
        return commandAllocator();
    }
    if (command == "prng") {
        return commandPrng(args);
    }
    if (command == "save") {
        return commandSave(args);
    }
    if (command == "load") {
        return commandLoad(args);
    }

    std::cout << "unknown command: " << command << "\n";
    return true;
}

void printCompletions(const std::string& prefix)
{
    const char* commands[] = {
        "init", "model", "circuit", "var", "equation", "compile", "pin", "lock", "unlock",
        "solve", "list", "stats", "allocator", "prng", "save", "load", "help", "quit"
    };
    bool printed = false;
    for (std::size_t index = 0U; index < sizeof(commands) / sizeof(commands[0]); ++index) {
        const std::string command(commands[index]);
        if (command.compare(0U, prefix.size(), prefix) == 0) {
            std::cout << command << " ";
            printed = true;
        }
    }
    if (printed) {
        std::cout << "\n";
    }
}

void loadHistory(std::vector<std::string>& history)
{
    const char* home = std::getenv("HOME");
    if (home == static_cast<const char*>(0)) {
        return;
    }

    const std::string path = std::string(home) + "/.semon_history";
    std::ifstream input(path.c_str());
    std::string line;
    while (std::getline(input, line)) {
        history.push_back(line);
    }
}

void saveHistory(const std::vector<std::string>& history)
{
    const char* home = std::getenv("HOME");
    if (home == static_cast<const char*>(0)) {
        return;
    }

    const std::string path = std::string(home) + "/.semon_history";
    std::ofstream output(path.c_str(), std::ios::out | std::ios::trunc);
    for (std::size_t index = 0U; index < history.size(); ++index) {
        output << history[index] << "\n";
    }
}

} // namespace

Shell::Shell()
    : running_(true)
{
    (void)g_allocator.initialize(64U);
    g_prng.seed(1ULL);
    ensureDefaultObjects();
}

int Shell::run(int argc, char** argv)
{
    if (argc > 1) {
        std::string command;
        for (int index = 1; index < argc; ++index) {
            if (index > 1) {
                command += " ";
            }
            command += argv[index];
        }
        return executeLine(command) ? 0 : 1;
    }

    std::vector<std::string> history;
    loadHistory(history);
    printHelp();

    while (running_) {
        std::cout << "semon> ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line == "\t") {
            printCompletions("");
            continue;
        }

        const bool keep_running = executeLine(line);
        if (!keep_running) {
            break;
        }

        if (!line.empty()) {
            history.push_back(line);
            saveHistory(history);
        }
    }

    saveHistory(history);
    return 0;
}

} // namespace core
} // namespace semon
