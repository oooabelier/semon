# semon - Deterministic Static Makefile
# All paths relative to build/ directory
# Obfuscated 3-char object names, explicit mappings

# ============================================================
# CONFIGURATION
# ============================================================
OS   ?= Linux
ARCH ?= AMD64
MODE ?= Debug

# OS codes
ifeq ($(OS),Windows)
  OS_CODE := w
endif
ifeq ($(OS),Linux)
  OS_CODE := l
endif
ifeq ($(OS),macOS)
  OS_CODE := m
endif
ifeq ($(OS),Android)
  OS_CODE := a
endif
ifeq ($(OS),iOS)
  OS_CODE := i
endif

# ARCH codes
ifeq ($(ARCH),AMD64)
  ARCH_CODE := x
endif
ifeq ($(ARCH),I386)
  ARCH_CODE := i
endif
ifeq ($(ARCH),ARM32)
  ARCH_CODE := a
endif
ifeq ($(ARCH),ARM64)
  ARCH_CODE := b
endif
ifeq ($(ARCH),RISCV)
  ARCH_CODE := r
endif

# MODE codes
ifeq ($(MODE),Debug)
  MODE_CODE := d
  CXXFLAGS_MODE := -O0 -g -DSEMON_DEBUG=1
endif
ifeq ($(MODE),Performance)
  MODE_CODE := p
  CXXFLAGS_MODE := -O2 -g -DNDEBUG
endif
ifeq ($(MODE),Release)
  MODE_CODE := r
  CXXFLAGS_MODE := -O3 -DNDEBUG
endif

TARGET_DIR := $(OS_CODE)$(ARCH_CODE)$(MODE_CODE)
OUT_DIR    := ./$(TARGET_DIR)
OBJ_DIR    := $(OUT_DIR)/obj

# ============================================================
# TOOLCHAIN
# ============================================================
CXX      := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -Werror -I../include $(CXXFLAGS_MODE)
AR       := ar
ARFLAGS  := rcs
RM       := rm -f
MKDIR    := mkdir -p

ifeq ($(OS),Windows)
  ifneq (,$(findstring mingw,$(shell $(CXX) --version 2>/dev/null)))
    CXXFLAGS += -D_WIN32 -DSEMON_WINDOWS=1
  else
    CXX      := clang++
    CXXFLAGS += -D_WIN32 -DSEMON_WINDOWS=1 -target x86_64-pc-windows-msvc
  endif
endif
ifeq ($(OS),Linux)
  CXXFLAGS += -DSEMON_LINUX=1 -pthread
endif
ifeq ($(OS),macOS)
  CXXFLAGS += -DSEMON_MACOS=1
endif
ifeq ($(OS),Android)
  CXXFLAGS += -DSEMON_ANDROID=1 -D__ANDROID_API__=24
endif
ifeq ($(OS),iOS)
  CXXFLAGS += -DSEMON_IOS=1 -target arm64-apple-ios13.0
endif

ifeq ($(ARCH),ARM64)
  CXXFLAGS += -DSEMON_ARM64=1
endif
ifeq ($(ARCH),ARM32)
  CXXFLAGS += -DSEMON_ARM32=1 -march=armv7-a
endif
ifeq ($(ARCH),RISCV)
  CXXFLAGS += -DSEMON_RISCV=1 -march=rv64gc
endif

# ============================================================
# SOURCE TO OBJECT MAPPING (OBFUSCATED 3-CHAR NAMES)
# ============================================================
# core/
OBJ_rzo := $(OBJ_DIR)/rzo.o  # src/core/PagedAllocator.cpp
OBJ_prn := $(OBJ_DIR)/prn.o  # src/core/DeterministicPrng.cpp
OBJ_reg := $(OBJ_DIR)/reg.o  # src/core/Registry.cpp
OBJ_shl := $(OBJ_DIR)/shl.o  # src/core/Shell.cpp
OBJ_tst := $(OBJ_DIR)/tst.o  # src/core/TestSupport.cpp
OBJ_dia := $(OBJ_DIR)/dia.o  # src/diagnostics/Diagnostics.cpp

# math/
OBJ_ast := $(OBJ_DIR)/ast.o  # src/math/MathModel.cpp
OBJ_par := $(OBJ_DIR)/par.o  # src/math/ExpressionParser.cpp
OBJ_syn := $(OBJ_DIR)/syn.o  # src/math/Synthesizer.cpp
OBJ_rob := $(OBJ_DIR)/rob.o  # src/math/ROBDD.cpp
OBJ_eso := $(OBJ_DIR)/eso.o  # src/math/ESOP.cpp
OBJ_cry := $(OBJ_DIR)/cry.o  # src/math/CryptoBuiltins.cpp
OBJ_mod := $(OBJ_DIR)/mod.o  # src/math/ModularArithmetic.cpp

# circuit/
OBJ_top := $(OBJ_DIR)/top.o  # src/circuit/CircuitTopology.cpp
OBJ_gat := $(OBJ_DIR)/gat.o  # src/circuit/Virtual3PinGate.cpp
OBJ_slv := $(OBJ_DIR)/slv.o  # src/circuit/Solver.cpp

# persistence/
OBJ_jsn := $(OBJ_DIR)/jsn.o  # src/persistence/JsonStreaming.cpp
OBJ_ses := $(OBJ_DIR)/ses.o  # src/persistence/SessionState.cpp

# test/
OBJ_cts := $(OBJ_DIR)/cts.o  # test/crypto_test.cpp

CORE_OBJS := $(OBJ_rzo) $(OBJ_prn) $(OBJ_reg) $(OBJ_shl) $(OBJ_tst) $(OBJ_dia)
MATH_OBJS := $(OBJ_ast) $(OBJ_par) $(OBJ_syn) $(OBJ_rob) $(OBJ_eso) $(OBJ_cry) $(OBJ_mod)
CIRC_OBJS := $(OBJ_top) $(OBJ_gat) $(OBJ_slv)
PERS_OBJS := $(OBJ_jsn) $(OBJ_ses)
TEST_OBJS := $(OBJ_tst) $(OBJ_cts)

ALL_OBJS := $(CORE_OBJS) $(MATH_OBJS) $(CIRC_OBJS) $(PERS_OBJS)

# ============================================================
# TARGETS
# ============================================================
STATIC_LIB := $(OUT_DIR)/libsemon_core.a
CLI_BIN    := $(OUT_DIR)/semon
TEST_BIN   := $(OUT_DIR)/semon_test

.PHONY: all clean directories cli test

all: directories $(STATIC_LIB) cli test

directories:
	$(MKDIR) $(OBJ_DIR)

# ============================================================
# STATIC LIBRARY
# ============================================================
$(STATIC_LIB): $(ALL_OBJS)
	$(AR) $(ARFLAGS) $@ $^

# ============================================================
# EXPLICIT COMPILATION RULES (NO WILDCARDS)
# ============================================================
$(OBJ_rzo): ../src/core/PagedAllocator.cpp ../include/core/PagedAllocator.hpp ../include/core/SystemSettings.hpp ../include/diagnostics/Diagnostics.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_prn): ../src/core/DeterministicPrng.cpp ../include/core/DeterministicPrng.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_reg): ../src/core/Registry.cpp ../include/core/Registry.hpp ../include/core/SystemSettings.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_shl): ../src/core/Shell.cpp ../include/core/Shell.hpp ../include/core/PagedAllocator.hpp ../include/core/DeterministicPrng.hpp ../include/core/Registry.hpp ../include/core/SystemSettings.hpp ../include/circuit/CircuitTopology.hpp ../include/circuit/Virtual3PinGate.hpp ../include/circuit/Solver.hpp ../include/diagnostics/Diagnostics.hpp ../include/math/ExpressionParser.hpp ../include/math/MathModel.hpp ../include/math/Synthesizer.hpp ../include/persistence/JsonStreaming.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_tst): ../src/core/TestSupport.cpp ../include/core/TestSupport.hpp ../include/core/PagedAllocator.hpp ../include/core/DeterministicPrng.hpp ../include/circuit/CircuitTopology.hpp ../include/circuit/Solver.hpp ../include/circuit/Virtual3PinGate.hpp ../include/math/ExpressionParser.hpp ../include/math/MathModel.hpp ../include/math/Synthesizer.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_dia): ../src/diagnostics/Diagnostics.cpp ../include/diagnostics/Diagnostics.hpp ../include/core/SystemSettings.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_ast): ../src/math/MathModel.cpp ../include/math/MathModel.hpp ../include/math/MathTypes.hpp ../include/math/ExpressionParser.hpp ../include/core/PagedAllocator.hpp ../include/core/SystemSettings.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_par): ../src/math/ExpressionParser.cpp ../include/math/ExpressionParser.hpp ../include/math/MathModel.hpp ../include/math/MathTypes.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_syn): ../src/math/Synthesizer.cpp ../include/math/Synthesizer.hpp ../include/math/MathModel.hpp ../include/math/MathTypes.hpp ../include/circuit/CircuitTopology.hpp ../include/circuit/Virtual3PinGate.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_mod): ../src/math/ModularArithmetic.cpp ../include/math/ModularArithmetic.hpp ../include/circuit/CircuitTopology.hpp ../include/circuit/Virtual3PinGate.hpp ../include/core/PagedAllocator.hpp ../include/diagnostics/Diagnostics.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_top): ../src/circuit/CircuitTopology.cpp ../include/circuit/CircuitTopology.hpp ../include/circuit/Virtual3PinGate.hpp ../include/core/PagedAllocator.hpp ../include/core/SystemSettings.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_gat): ../src/circuit/Virtual3PinGate.cpp ../include/circuit/Virtual3PinGate.hpp ../include/circuit/CircuitTopology.hpp ../include/math/MathTypes.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_slv): ../src/circuit/Solver.cpp ../include/circuit/Solver.hpp ../include/circuit/CircuitTopology.hpp ../include/circuit/Virtual3PinGate.hpp ../include/core/SystemSettings.hpp ../include/diagnostics/Diagnostics.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_jsn): ../src/persistence/JsonStreaming.cpp ../include/persistence/JsonStreaming.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_ses): ../src/persistence/SessionState.cpp ../include/persistence/SessionState.hpp ../include/persistence/JsonStreaming.hpp ../include/math/MathModel.hpp ../include/math/MathTypes.hpp ../include/circuit/CircuitTopology.hpp ../include/core/DeterministicPrng.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_rob): ../src/math/ROBDD.cpp ../include/math/ROBDD.hpp ../include/math/MathTypes.hpp ../include/core/PagedAllocator.hpp ../include/core/SystemSettings.hpp ../include/diagnostics/Diagnostics.hpp ../include/circuit/CircuitTopology.hpp ../include/circuit/Virtual3PinGate.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_eso): ../src/math/ESOP.cpp ../include/math/ESOP.hpp ../include/math/MathTypes.hpp ../include/core/PagedAllocator.hpp ../include/core/SystemSettings.hpp ../include/diagnostics/Diagnostics.hpp ../include/circuit/CircuitTopology.hpp ../include/circuit/Virtual3PinGate.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_cry): ../src/math/CryptoBuiltins.cpp ../include/math/CryptoBuiltins.hpp ../include/math/MathModel.hpp ../include/math/MathTypes.hpp ../include/core/PagedAllocator.hpp ../include/circuit/CircuitTopology.hpp ../include/circuit/Virtual3PinGate.hpp ../include/diagnostics/Diagnostics.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_cts): ../test/crypto_test.cpp ../include/core/TestSupport.hpp ../include/core/PagedAllocator.hpp ../include/core/DeterministicPrng.hpp ../include/math/MathModel.hpp ../include/math/ExpressionParser.hpp ../include/math/Synthesizer.hpp ../include/math/CryptoBuiltins.hpp ../include/math/ROBDD.hpp ../include/math/ESOP.hpp ../include/circuit/CircuitTopology.hpp ../include/circuit/Solver.hpp ../include/circuit/Virtual3PinGate.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ============================================================
# CLI BINARY
# ============================================================
CLI_OBJ := $(OUT_DIR)/cli.o

$(CLI_OBJ): ../src/main.cpp ../include/core/Shell.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

cli: $(CLI_BIN)

$(CLI_BIN): $(CLI_OBJ) $(STATIC_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $(CLI_OBJ) -L$(OUT_DIR) -lsemon_core

# ============================================================
# TEST BINARY
# ============================================================
TEST_OBJ := $(OUT_DIR)/tent.o

$(TEST_OBJ): ../src/test_entry.cpp ../include/core/TestSupport.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_BIN)

$(TEST_BIN): $(TEST_OBJ) $(OBJ_cts) $(STATIC_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_OBJ) $(OBJ_cts) -L$(OUT_DIR) -lsemon_core

# ============================================================
# CLEAN
# ============================================================
clean:
	$(RM) -r $(OUT_DIR)

# ============================================================
# STATIC ANALYSIS (clang-tidy / cppcheck)
# ============================================================
ANALYSIS_SRCS := ../src/core/PagedAllocator.cpp \
                 ../src/core/DeterministicPrng.cpp \
                 ../src/core/Registry.cpp \
                 ../src/core/Shell.cpp \
                 ../src/core/TestSupport.cpp \
                 ../src/diagnostics/Diagnostics.cpp \
                 ../src/math/MathModel.cpp \
                 ../src/math/ExpressionParser.cpp \
                 ../src/math/Synthesizer.cpp \
                 ../src/circuit/CircuitTopology.cpp \
                 ../src/circuit/Virtual3PinGate.cpp \
                 ../src/circuit/Solver.cpp \
                 ../src/persistence/JsonStreaming.cpp \
                 ../src/persistence/SessionState.cpp

tidy:
	clang-tidy $(ANALYSIS_SRCS) -- -std=c++11 -I../include $(CXXFLAGS_MODE)

cppcheck:
	cppcheck --std=c++11 --enable=all -I../include $(ANALYSIS_SRCS)

# ============================================================
# HELP
# ============================================================
help:
	@echo "Usage: make OS=<OS> ARCH=<ARCH> MODE=<MODE> [target]"
	@echo ""
	@echo "OS:     Windows, Linux, macOS, Android, iOS"
	@echo "ARCH:   AMD64, I386, ARM32, ARM64, RISCV"
	@echo "MODE:   Debug, Performance, Release"
	@echo ""
	@echo "Targets: all, cli, test, clean, tidy, cppcheck, help"
	@echo ""
	@echo "Output directory: build/$(TARGET_DIR)/"
