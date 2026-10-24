#ifndef TACCO_SCOPE_HPP
#define TACCO_SCOPE_HPP

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <iostream>
#include "symbol.hpp"

namespace tacco {

enum class ScopeKind {
    GLOBAL,
    FUNCTION,
    BLOCK,
    CLASS
};

const char* scope_kind_to_string(ScopeKind kind);

class Scope;
using ScopePtr = std::shared_ptr<Scope>;

class Scope : public std::enable_shared_from_this<Scope> {
public:
    ScopeKind kind;
    ScopePtr parent;
    std::string name; // Name of function/class/block
    std::vector<ScopePtr> children;
    std::unordered_map<std::string, SymbolPtr> symbols;
    std::vector<SymbolPtr> ordered_symbols;

    explicit Scope(ScopeKind k, ScopePtr p = nullptr, std::string n = "")
        : kind(k), parent(std::move(p)), name(std::move(n)) {}

    bool insert(SymbolPtr symbol);
    SymbolPtr lookup_current(const std::string& name) const;
    SymbolPtr lookup(const std::string& name) const;

    ScopePtr get_parent() const { return parent; }
    ScopePtr add_child(ScopeKind child_kind, const std::string& child_name = "");

    void dump(std::ostream& os, int indent = 0) const;
};

} // namespace tacco

#endif // TACCO_SCOPE_HPP
