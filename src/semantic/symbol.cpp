#include "symbol.hpp"
#include <sstream>

namespace tacco {

const char* symbol_kind_to_string(SymbolKind kind) {
    switch (kind) {
        case SymbolKind::VARIABLE:  return "variable";
        case SymbolKind::PARAMETER: return "parameter";
        case SymbolKind::FUNCTION:  return "function";
        case SymbolKind::CLASS:     return "class";
        case SymbolKind::FIELD:     return "field";
    }
    return "unknown_symbol";
}

std::string Symbol::to_string() const {
    std::ostringstream oss;
    oss << symbol_kind_to_string(kind) << " '" << name << "' : "
        << (type ? type->to_string() : "unknown") << " [" << location.to_string() << "]";
    if (is_function()) {
        oss << " (params: " << parameters.size() << ")";
    } else if (is_class()) {
        oss << " (fields: " << ordered_fields.size() << ")";
    }
    return oss.str();
}

} // namespace tacco
