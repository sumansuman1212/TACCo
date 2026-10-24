#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "lexer/lexer_driver.hpp"
#include "parser/parser_driver.hpp"
#include "semantic/symbol_table.hpp"
#include "semantic/type_checker.hpp"
#include "ir/ir_builder.hpp"
#include "cfg/cfg.hpp"
#include "optimizer/optimizer.hpp"
#include "codegen/riscv_codegen.hpp"

static void print_usage(const char* prog_name) {
    std::cout << "TACCo — Totally Another C Compiler (v1.0.0-release)\n"
              << "Usage: " << prog_name << " [options] <source-file>\n\n"
              << "Options:\n"
              << "  -t, --dump-tokens    Tokenize source file and print token stream\n"
              << "  -a, --dump-ast       Parse source file and print Abstract Syntax Tree (AST)\n"
              << "  -s, --dump-symbols   Build symbol table and print scopes/symbols\n"
              << "  -c, --check          Perform syntax and semantic type-checking only\n"
              << "  -i, --dump-ir        Generate and print Three Address Code (3AC) IR\n"
              << "      --dump-cfg       Generate and print Control Flow Graph (DOT format)\n"
              << "  -O, --optimize       Enable compiler optimization passes\n"
              << "  -S, --emit-asm       Emit 64-bit RISC-V (RV64) assembly code\n"
              << "  -o <file>            Specify output file name\n"
              << "  -v, --version        Display compiler version\n"
              << "  -h, --help           Display this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string input_file;
    std::string output_file;
    bool dump_tokens = false;
    bool dump_ast = false;
    bool dump_symbols = false;
    bool check_only = false;
    bool dump_ir = false;
    bool dump_cfg = false;
    bool optimize = false;
    bool emit_asm = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "TACCo version 1.0.0 (End-to-End RV64 Compiler)\n";
            return 0;
        } else if (arg == "-t" || arg == "--dump-tokens") {
            dump_tokens = true;
        } else if (arg == "-a" || arg == "--dump-ast") {
            dump_ast = true;
        } else if (arg == "-s" || arg == "--dump-symbols") {
            dump_symbols = true;
        } else if (arg == "-c" || arg == "--check") {
            check_only = true;
        } else if (arg == "-i" || arg == "--dump-ir") {
            dump_ir = true;
        } else if (arg == "--dump-cfg") {
            dump_cfg = true;
        } else if (arg == "-O" || arg == "--optimize") {
            optimize = true;
        } else if (arg == "-S" || arg == "--emit-asm") {
            emit_asm = true;
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                std::cerr << "error: -o requires an output filename argument\n";
                return 1;
            }
        } else if (arg.rfind("-", 0) == 0) {
            std::cerr << "error: unknown option '" << arg << "'\n";
            print_usage(argv[0]);
            return 1;
        } else {
            input_file = arg;
        }
    }

    if (input_file.empty()) {
        std::cerr << "error: no input file provided\n";
        print_usage(argv[0]);
        return 1;
    }

    // Stage 1: Lexical Analysis
    if (dump_tokens) {
        tacco::LexerDriver lexer;
        if (!lexer.open_file(input_file)) {
            std::cerr << "error: could not open source file '" << input_file << "'\n";
            return 1;
        }

        std::vector<tacco::Token> tokens = lexer.tokenize_all();
        std::cout << "=== TACCo Token Stream: " << input_file << " ===\n";
        for (const auto& tok : tokens) {
            std::cout << tok.to_string() << "\n";
        }

        if (lexer.has_errors()) {
            std::cerr << "\nLexical Errors (" << lexer.get_error_messages().size() << "):\n";
            for (const auto& err : lexer.get_error_messages()) {
                std::cerr << "  " << err << "\n";
            }
            return 1;
        }
        return 0;
    }

    // Stage 2: Parsing & AST Construction
    tacco::ParserDriver parser;
    bool parse_ok = parser.parse_file(input_file);

    if (dump_ast) {
        std::cout << "=== TACCo Abstract Syntax Tree (AST): " << input_file << " ===\n";
        if (parser.get_ast()) {
            parser.get_ast()->dump(std::cout);
        } else {
            std::cout << "(No AST generated due to parse errors)\n";
        }
    }

    if (!parse_ok || parser.has_errors()) {
        std::cerr << "\nParser Errors Encountered (" << parser.get_error_messages().size() << "):\n";
        for (const auto& err : parser.get_error_messages()) {
            std::cerr << "  " << err << "\n";
        }
        return 1;
    }

    // Stage 3: Symbol Table & Semantic Analysis
    tacco::TypeChecker type_checker;
    bool sem_ok = type_checker.check(*parser.get_ast());

    if (dump_symbols) {
        std::cout << "=== TACCo Symbol Table & Scopes: " << input_file << " ===\n";
        type_checker.get_symbol_table().dump(std::cout);
    }

    if (!sem_ok || type_checker.has_errors()) {
        std::cerr << "\nSemantic Errors Encountered (" << type_checker.get_error_messages().size() << "):\n";
        for (const auto& err : type_checker.get_error_messages()) {
            std::cerr << "  " << err << "\n";
        }
        return 1;
    }

    if (check_only) {
        std::cout << "Syntax and semantic check succeeded for '" << input_file << "'. 0 errors.\n";
        return 0;
    }

    // Stage 4: Three Address Code (3AC) Intermediate Representation Generation
    tacco::IRBuilder ir_builder;
    auto ir_program = ir_builder.build(*parser.get_ast());
    if (!ir_program) {
        std::cerr << "error: failed to generate 3AC intermediate representation\n";
        return 1;
    }

    // Stage 5: Control Flow Graph & Basic Block Partitioning
    auto pcfg = tacco::ProgramCFG::build_from_program(*ir_program);

    if (dump_cfg) {
        std::cout << pcfg->to_dot();
        return 0;
    }

    // Stage 6: Optimization Passes (Constant Folding, Propagation, DCE, CSE)
    if (optimize) {
        tacco::Optimizer optimizer;
        optimizer.optimize(*ir_program);
    }

    if (dump_ir) {
        std::cout << "=== TACCo 3AC Intermediate Representation: " << input_file << " ===\n";
        ir_program->dump(std::cout);
        return 0;
    }

    // Stage 7: Register Allocation & RISC-V (RV64) Code Generation
    tacco::RISCVCodeGenerator codegen;
    std::string asm_output = codegen.generate(*ir_program);

    if (!output_file.empty()) {
        std::ofstream out(output_file);
        if (!out.is_open()) {
            std::cerr << "error: could not open output file '" << output_file << "'\n";
            return 1;
        }
        out << asm_output;
        out.close();
    } else if (emit_asm || (!dump_ast && !dump_symbols)) {
        std::cout << asm_output;
    }

    return 0;
}
