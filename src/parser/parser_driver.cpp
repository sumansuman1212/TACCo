#include "parser_driver.hpp"
#include "parser.hpp"
#include <iostream>

extern int tacco_yyparse(tacco::ParserDriver& driver, void* scanner);

namespace tacco {

ParserDriver::ParserDriver() = default;
ParserDriver::~ParserDriver() = default;

void ParserDriver::record_error(const std::string& msg, const SourceLocation& loc) {
    std::string formatted = "syntax error: " + msg + " at " + loc.to_string();
    error_messages_.push_back(formatted);
}

bool ParserDriver::parse_file(const std::string& filepath) {
    filename_ = filepath;
    error_messages_.clear();
    program_ast_.reset();

    if (!lexer_.open_file(filepath)) {
        record_error("Could not open source file '" + filepath + "'", SourceLocation(filepath, 1, 1));
        return false;
    }

    int result = tacco_yyparse(*this, lexer_.get_scanner());
    return (result == 0) && !has_errors();
}

bool ParserDriver::parse_string(const std::string& source, const std::string& source_name) {
    filename_ = source_name;
    error_messages_.clear();
    program_ast_.reset();

    lexer_.set_input_string(source, source_name);

    int result = tacco_yyparse(*this, lexer_.get_scanner());
    return (result == 0) && !has_errors();
}

} // namespace tacco
