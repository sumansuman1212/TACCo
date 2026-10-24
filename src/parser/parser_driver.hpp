#ifndef TACCO_PARSER_DRIVER_HPP
#define TACCO_PARSER_DRIVER_HPP

#include <string>
#include <vector>
#include <memory>
#include "../common/location.hpp"
#include "../ast/ast.hpp"
#include "../ast/declarations.hpp"
#include "../lexer/lexer_driver.hpp"

namespace tacco {

class ParserDriver {
public:
    ParserDriver();
    ~ParserDriver();

    bool parse_file(const std::string& filepath);
    bool parse_string(const std::string& source, const std::string& source_name = "<string>");

    std::unique_ptr<ProgramNode>& get_ast() { return program_ast_; }
    void set_ast(std::unique_ptr<ProgramNode> ast) { program_ast_ = std::move(ast); }

    const std::string& get_filename() const { return filename_; }
    bool has_errors() const { return !error_messages_.empty(); }
    const std::vector<std::string>& get_error_messages() const { return error_messages_; }

    void record_error(const std::string& msg, const SourceLocation& loc);
    LexerDriver& get_lexer() { return lexer_; }

private:
    std::string filename_;
    LexerDriver lexer_;
    std::unique_ptr<ProgramNode> program_ast_;
    std::vector<std::string> error_messages_;
};

} // namespace tacco

#endif // TACCO_PARSER_DRIVER_HPP
