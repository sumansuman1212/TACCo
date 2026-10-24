#include "ast.hpp"
#include <sstream>

namespace tacco {

std::string ASTNode::dump_string(int indent) const {
    std::ostringstream oss;
    dump(oss, indent);
    return oss.str();
}

} // namespace tacco
