CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -Isrc -Isrc/lexer -Isrc/parser -Isrc/ast -Isrc/semantic -Isrc/ir -Isrc/cfg -Isrc/optimizer -Isrc/codegen -Ibuild
FLEX ?= flex
BISON ?= bison

BIN_DIR = bin
BUILD_DIR = build
SRC_DIR = src

LEXER_L = $(SRC_DIR)/lexer/lexer.l
LEXER_CPP = $(BUILD_DIR)/lexer.cpp

PARSER_Y = $(SRC_DIR)/parser/parser.y
PARSER_CPP = $(BUILD_DIR)/parser.cpp
PARSER_HPP = $(BUILD_DIR)/parser.hpp

CORE_OBJS = $(BUILD_DIR)/token.o \
            $(BUILD_DIR)/lexer_driver.o \
            $(BUILD_DIR)/lexer.o \
            $(BUILD_DIR)/types.o \
            $(BUILD_DIR)/ast.o \
            $(BUILD_DIR)/ast_dump.o \
            $(BUILD_DIR)/parser_driver.o \
            $(BUILD_DIR)/parser.o \
            $(BUILD_DIR)/symbol.o \
            $(BUILD_DIR)/scope.o \
            $(BUILD_DIR)/symbol_table.o \
            $(BUILD_DIR)/type_checker.o \
            $(BUILD_DIR)/ir.o \
            $(BUILD_DIR)/ir_builder.o \
            $(BUILD_DIR)/cfg.o \
            $(BUILD_DIR)/optimizer.o \
            $(BUILD_DIR)/regalloc.o \
            $(BUILD_DIR)/riscv_codegen.o

MAIN_OBJS = $(BUILD_DIR)/main.o $(CORE_OBJS)
TEST_LEXER_OBJS = $(BUILD_DIR)/test_lexer.o $(CORE_OBJS)
TEST_PARSER_OBJS = $(BUILD_DIR)/test_parser.o $(CORE_OBJS)
TEST_SYMTAB_OBJS = $(BUILD_DIR)/test_symbol_table.o $(CORE_OBJS)
TEST_SEMANTIC_OBJS = $(BUILD_DIR)/test_semantic.o $(CORE_OBJS)
TEST_IR_OBJS = $(BUILD_DIR)/test_ir.o $(CORE_OBJS)
TEST_CFG_OBJS = $(BUILD_DIR)/test_cfg.o $(CORE_OBJS)
TEST_OPT_OBJS = $(BUILD_DIR)/test_optimizer.o $(CORE_OBJS)
TEST_CODEGEN_OBJS = $(BUILD_DIR)/test_codegen.o $(CORE_OBJS)
TEST_E2E_OBJS = $(BUILD_DIR)/test_e2e.o $(CORE_OBJS)

TARGET = $(BIN_DIR)/tacco
TEST_LEXER_TARGET = $(BIN_DIR)/test_lexer
TEST_PARSER_TARGET = $(BIN_DIR)/test_parser
TEST_SYMTAB_TARGET = $(BIN_DIR)/test_symbol_table
TEST_SEMANTIC_TARGET = $(BIN_DIR)/test_semantic
TEST_IR_TARGET = $(BIN_DIR)/test_ir
TEST_CFG_TARGET = $(BIN_DIR)/test_cfg
TEST_OPT_TARGET = $(BIN_DIR)/test_optimizer
TEST_CODEGEN_TARGET = $(BIN_DIR)/test_codegen
TEST_E2E_TARGET = $(BIN_DIR)/test_e2e

ALL_TEST_TARGETS = $(TEST_LEXER_TARGET) $(TEST_PARSER_TARGET) $(TEST_SYMTAB_TARGET) $(TEST_SEMANTIC_TARGET) $(TEST_IR_TARGET) $(TEST_CFG_TARGET) $(TEST_OPT_TARGET) $(TEST_CODEGEN_TARGET) $(TEST_E2E_TARGET)

.PHONY: all clean test test-lexer test-parser test-symtab test-semantic test-ir test-cfg test-opt test-codegen test-e2e test-qemu dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BIN_DIR) $(BUILD_DIR)

$(PARSER_CPP) $(PARSER_HPP): $(PARSER_Y) | dirs
	$(BISON) -d -o $(PARSER_CPP) $(PARSER_Y)

$(LEXER_CPP): $(LEXER_L) $(PARSER_HPP) | dirs
	$(FLEX) -o $(LEXER_CPP) $(LEXER_L)

$(BUILD_DIR)/parser.o: $(PARSER_CPP) | dirs
	$(CXX) $(CXXFLAGS) -Wno-unused-function -Wno-unused-variable -c $< -o $@

$(BUILD_DIR)/lexer.o: $(LEXER_CPP) | dirs
	$(CXX) $(CXXFLAGS) -Wno-unused-function -Wno-sign-compare -c $< -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp $(PARSER_HPP) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/token.o: $(SRC_DIR)/lexer/token.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/lexer_driver.o: $(SRC_DIR)/lexer/lexer_driver.cpp $(PARSER_HPP) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/types.o: $(SRC_DIR)/ast/types.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/ast.o: $(SRC_DIR)/ast/ast.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump.o: $(SRC_DIR)/ast/ast_dump.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_driver.o: $(SRC_DIR)/parser/parser_driver.cpp $(PARSER_HPP) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol.o: $(SRC_DIR)/semantic/symbol.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/scope.o: $(SRC_DIR)/semantic/scope.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table.o: $(SRC_DIR)/semantic/symbol_table.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker.o: $(SRC_DIR)/semantic/type_checker.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/ir.o: $(SRC_DIR)/ir/ir.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/ir_builder.o: $(SRC_DIR)/ir/ir_builder.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/cfg.o: $(SRC_DIR)/cfg/cfg.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/optimizer.o: $(SRC_DIR)/optimizer/optimizer.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/regalloc.o: $(SRC_DIR)/codegen/regalloc.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/riscv_codegen.o: $(SRC_DIR)/codegen/riscv_codegen.cpp | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_lexer.o: tests/lexer/test_lexer.cpp $(PARSER_HPP) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser.o: tests/parser/test_parser.cpp $(PARSER_HPP) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_symbol_table.o: tests/semantic/test_symbol_table.cpp $(PARSER_HPP) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_semantic.o: tests/semantic/test_semantic.cpp $(PARSER_HPP) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ir.o: tests/ir/test_ir.cpp $(PARSER_HPP) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_cfg.o: tests/cfg/test_cfg.cpp $(PARSER_HPP) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_optimizer.o: tests/optimizer/test_optimizer.cpp $(PARSER_HPP) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_codegen.o: tests/codegen/test_codegen.cpp $(PARSER_HPP) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_e2e.o: tests/e2e/test_e2e.cpp $(PARSER_HPP) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(MAIN_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_LEXER_TARGET): $(TEST_LEXER_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_PARSER_TARGET): $(TEST_PARSER_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_SYMTAB_TARGET): $(TEST_SYMTAB_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_SEMANTIC_TARGET): $(TEST_SEMANTIC_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_IR_TARGET): $(TEST_IR_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_CFG_TARGET): $(TEST_CFG_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_OPT_TARGET): $(TEST_OPT_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_CODEGEN_TARGET): $(TEST_CODEGEN_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_E2E_TARGET): $(TEST_E2E_OBJS) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(ALL_TEST_TARGETS)
	@echo "Running Lexer unit tests..."
	@./$(TEST_LEXER_TARGET)
	@echo ""
	@echo "Running Parser unit tests..."
	@./$(TEST_PARSER_TARGET)
	@echo ""
	@echo "Running Symbol Table unit tests..."
	@./$(TEST_SYMTAB_TARGET)
	@echo ""
	@echo "Running Semantic unit tests..."
	@./$(TEST_SEMANTIC_TARGET)
	@echo ""
	@echo "Running Three Address Code IR unit tests..."
	@./$(TEST_IR_TARGET)
	@echo ""
	@echo "Running Basic Blocks & CFG unit tests..."
	@./$(TEST_CFG_TARGET)
	@echo ""
	@echo "Running Optimizer unit tests..."
	@./$(TEST_OPT_TARGET)
	@echo ""
	@echo "Running RISC-V Codegen unit tests..."
	@./$(TEST_CODEGEN_TARGET)
	@echo ""
	@echo "Running End-to-End Compiler unit tests..."
	@./$(TEST_E2E_TARGET)

test-lexer: $(TEST_LEXER_TARGET)
	@./$(TEST_LEXER_TARGET)

test-parser: $(TEST_PARSER_TARGET)
	@./$(TEST_PARSER_TARGET)

test-symtab: $(TEST_SYMTAB_TARGET)
	@./$(TEST_SYMTAB_TARGET)

test-semantic: $(TEST_SEMANTIC_TARGET)
	@./$(TEST_SEMANTIC_TARGET)

test-ir: $(TEST_IR_TARGET)
	@./$(TEST_IR_TARGET)

test-cfg: $(TEST_CFG_TARGET)
	@./$(TEST_CFG_TARGET)

test-opt: $(TEST_OPT_TARGET)
	@./$(TEST_OPT_TARGET)

test-codegen: $(TEST_CODEGEN_TARGET)
	@./$(TEST_CODEGEN_TARGET)

test-e2e: $(TEST_E2E_TARGET)
	@./$(TEST_E2E_TARGET)

test-qemu: $(TARGET)
	@./test_qemu.sh

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) *.s *.out
