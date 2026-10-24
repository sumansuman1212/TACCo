#ifndef TACCO_TYPES_HPP
#define TACCO_TYPES_HPP

#include <string>
#include <memory>
#include <vector>

namespace tacco {

enum class TypeKind {
    INT,
    CHAR,
    VOID,
    POINTER,
    ARRAY,
    CLASS,
    FUNCTION
};

class Type;
using TypePtr = std::shared_ptr<Type>;

class Type {
public:
    TypeKind kind;
    std::string name; // For primitive type names or class names
    TypePtr base_type; // For pointer, array element, or function return type
    int array_size = 0; // For array types
    std::vector<TypePtr> param_types; // For function signatures

    Type(TypeKind k, std::string n = "")
        : kind(k), name(std::move(n)), base_type(nullptr), array_size(0) {}

    static TypePtr make_int() {
        return std::make_shared<Type>(TypeKind::INT, "int");
    }

    static TypePtr make_char() {
        return std::make_shared<Type>(TypeKind::CHAR, "char");
    }

    static TypePtr make_void() {
        return std::make_shared<Type>(TypeKind::VOID, "void");
    }

    static TypePtr make_pointer(TypePtr base) {
        auto t = std::make_shared<Type>(TypeKind::POINTER);
        t->base_type = std::move(base);
        return t;
    }

    static TypePtr make_array(TypePtr element_type, int size) {
        auto t = std::make_shared<Type>(TypeKind::ARRAY);
        t->base_type = std::move(element_type);
        t->array_size = size;
        return t;
    }

    static TypePtr make_class(const std::string& class_name) {
        return std::make_shared<Type>(TypeKind::CLASS, class_name);
    }

    static TypePtr make_function(TypePtr return_type, std::vector<TypePtr> params) {
        auto t = std::make_shared<Type>(TypeKind::FUNCTION);
        t->base_type = std::move(return_type);
        t->param_types = std::move(params);
        return t;
    }

    bool is_int() const { return kind == TypeKind::INT; }
    bool is_char() const { return kind == TypeKind::CHAR; }
    bool is_void() const { return kind == TypeKind::VOID; }
    bool is_pointer() const { return kind == TypeKind::POINTER; }
    bool is_array() const { return kind == TypeKind::ARRAY; }
    bool is_class() const { return kind == TypeKind::CLASS; }
    bool is_function() const { return kind == TypeKind::FUNCTION; }

    bool is_integral() const { return is_int() || is_char(); }

    bool equals(const Type& other) const;
    std::string to_string() const;
};

} // namespace tacco

#endif // TACCO_TYPES_HPP
