#include "types.hpp"
#include <sstream>

namespace tacco {

bool Type::equals(const Type& other) const {
    if (kind != other.kind) return false;
    switch (kind) {
        case TypeKind::INT:
        case TypeKind::CHAR:
        case TypeKind::VOID:
            return true;
        case TypeKind::CLASS:
            return name == other.name;
        case TypeKind::POINTER:
            return base_type && other.base_type && base_type->equals(*other.base_type);
        case TypeKind::ARRAY:
            return array_size == other.array_size &&
                   base_type && other.base_type && base_type->equals(*other.base_type);
        case TypeKind::FUNCTION:
            if (!base_type || !other.base_type || !base_type->equals(*other.base_type))
                return false;
            if (param_types.size() != other.param_types.size())
                return false;
            for (size_t i = 0; i < param_types.size(); ++i) {
                if (!param_types[i]->equals(*other.param_types[i]))
                    return false;
            }
            return true;
    }
    return false;
}

std::string Type::to_string() const {
    switch (kind) {
        case TypeKind::INT:     return "int";
        case TypeKind::CHAR:    return "char";
        case TypeKind::VOID:    return "void";
        case TypeKind::CLASS:   return "class " + name;
        case TypeKind::POINTER: return base_type ? base_type->to_string() + "*" : "void*";
        case TypeKind::ARRAY:   return (base_type ? base_type->to_string() : "void") + "[" + std::to_string(array_size) + "]";
        case TypeKind::FUNCTION: {
            std::ostringstream oss;
            oss << (base_type ? base_type->to_string() : "void") << "(";
            for (size_t i = 0; i < param_types.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << param_types[i]->to_string();
            }
            oss << ")";
            return oss.str();
        }
    }
    return "unknown_type";
}

} // namespace tacco
