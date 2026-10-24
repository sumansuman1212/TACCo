#include "scope.hpp"
#include <iomanip>

namespace tacco {

const char* scope_kind_to_string(ScopeKind kind) {
    switch (kind) {
        case ScopeKind::GLOBAL:   return "global";
        case ScopeKind::FUNCTION: return "function";
        case ScopeKind::BLOCK:    return "block";
        case ScopeKind::CLASS:    return "class";
    }
    return "unknown_scope";
}

static void print_scope_indent(std::ostream& os, int indent) {
    for (int i = 0; i < indent; ++i) {
        os << "  ";
    }
}

bool Scope::insert(SymbolPtr symbol) {
    if (!symbol || symbols.find(symbol->name) != symbols.end()) {
        return false;
    }
    symbols[symbol->name] = symbol;
    ordered_symbols.push_back(symbol);
    return true;
}

SymbolPtr Scope::lookup_current(const std::string& name) const {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second;
    }
    return nullptr;
}

SymbolPtr Scope::lookup(const std::string& name) const {
    auto sym = lookup_current(name);
    if (sym) {
        return sym;
    }
    if (parent) {
        return parent->lookup(name);
    }
    return nullptr;
}

ScopePtr Scope::add_child(ScopeKind child_kind, const std::string& child_name) {
    auto child = std::make_shared<Scope>(child_kind, shared_from_this(), child_name);
    children.push_back(child);
    return child;
}

void Scope::dump(std::ostream& os, int indent) const {
    print_scope_indent(os, indent);
    os << "Scope (" << scope_kind_to_string(kind)
       << (name.empty() ? "" : ": " + name)
       << ") with " << ordered_symbols.size() << " symbols:\n";

    for (const auto& sym : ordered_symbols) {
        print_scope_indent(os, indent + 1);
        os << sym->to_string() << "\n";
    }

    for (const auto& child : children) {
        child->dump(os, indent + 1);
    }
}

} // namespace tacco
