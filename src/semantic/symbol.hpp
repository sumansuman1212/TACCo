#ifndef TACCO_SYMBOL_HPP
#define TACCO_SYMBOL_HPP

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../common/location.hpp"
#include "../ast/types.hpp"

namespace tacco {

enum class SymbolKind {
    VARIABLE,
    PARAMETER,
    FUNCTION,
    CLASS,
    FIELD
};

const char* symbol_kind_to_string(SymbolKind kind);

class Symbol;
using SymbolPtr = std::shared_ptr<Symbol>;

class Symbol {
public:
    std::string name;
    SymbolKind kind;
    TypePtr type;
    SourceLocation location;

    // For function symbols
    std::vector<SymbolPtr> parameters;

    // For class symbols
    std::unordered_map<std::string, SymbolPtr> fields;
    std::vector<SymbolPtr> ordered_fields;

    // Memory / layout information (populated during layout / codegen)
    int offset = 0;
    int size = 0;

    Symbol(std::string n, SymbolKind k, TypePtr t, SourceLocation loc)
        : name(std::move(n)), kind(k), type(std::move(t)), location(std::move(loc)) {}

    static SymbolPtr make_variable(const std::string& name, TypePtr type, SourceLocation loc) {
        return std::make_shared<Symbol>(name, SymbolKind::VARIABLE, std::move(type), std::move(loc));
    }

    static SymbolPtr make_parameter(const std::string& name, TypePtr type, SourceLocation loc) {
        return std::make_shared<Symbol>(name, SymbolKind::PARAMETER, std::move(type), std::move(loc));
    }

    static SymbolPtr make_function(const std::string& name, TypePtr return_type, SourceLocation loc) {
        return std::make_shared<Symbol>(name, SymbolKind::FUNCTION, std::move(return_type), std::move(loc));
    }

    static SymbolPtr make_class(const std::string& name, SourceLocation loc) {
        return std::make_shared<Symbol>(name, SymbolKind::CLASS, Type::make_class(name), std::move(loc));
    }

    static SymbolPtr make_field(const std::string& name, TypePtr type, SourceLocation loc) {
        return std::make_shared<Symbol>(name, SymbolKind::FIELD, std::move(type), std::move(loc));
    }

    bool is_variable() const { return kind == SymbolKind::VARIABLE; }
    bool is_parameter() const { return kind == SymbolKind::PARAMETER; }
    bool is_function() const { return kind == SymbolKind::FUNCTION; }
    bool is_class() const { return kind == SymbolKind::CLASS; }
    bool is_field() const { return kind == SymbolKind::FIELD; }

    bool add_field(SymbolPtr field) {
        if (!field || fields.find(field->name) != fields.end()) {
            return false;
        }
        fields[field->name] = field;
        ordered_fields.push_back(field);
        return true;
    }

    SymbolPtr get_field(const std::string& field_name) const {
        auto it = fields.find(field_name);
        if (it != fields.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::string to_string() const;
};

} // namespace tacco

#endif // TACCO_SYMBOL_HPP
