#ifndef TACCO_LEXER_DRIVER_HPP
#define TACCO_LEXER_DRIVER_HPP

#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include "token.hpp"

namespace tacco {

class LexerDriver {
public:
    LexerDriver();
    ~LexerDriver();

    bool open_file(const std::string& filepath);
    void set_input_string(const std::string& input, const std::string& source_name = "<string>");

    Token next_token();
    std::vector<Token> tokenize_all();

    const std::string& get_filename() const { return filename_; }
    int get_line() const { return current_line_; }
    int get_column() const { return current_column_; }

    void set_location(int line, int col) {
        current_line_ = line;
        current_column_ = col;
    }

    void advance_column(int len) {
        current_column_ += len;
    }

    void newline() {
        current_line_++;
        current_column_ = 1;
    }

    void record_error(const std::string& msg, const SourceLocation& loc);
    bool has_errors() const { return !error_messages_.empty(); }
    const std::vector<std::string>& get_error_messages() const { return error_messages_; }

    void* get_scanner() { return scanner_; }

private:
    std::string filename_;
    int current_line_ = 1;
    int current_column_ = 1;
    void* scanner_ = nullptr;
    std::vector<std::string> error_messages_;
    FILE* file_handle_ = nullptr;
    std::string string_buffer_;
    void* buffer_state_ = nullptr;
};

} // namespace tacco

#endif // TACCO_LEXER_DRIVER_HPP
