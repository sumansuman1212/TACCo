#ifndef TACCO_LOCATION_HPP
#define TACCO_LOCATION_HPP

#include <string>
#include <sstream>

namespace tacco {

struct SourceLocation {
    std::string filename;
    int line;
    int column;

    SourceLocation() : filename(""), line(1), column(1) {}
    SourceLocation(std::string fn, int l, int c)
        : filename(std::move(fn)), line(l), column(c) {}

    std::string to_string() const {
        std::ostringstream oss;
        if (!filename.empty()) {
            oss << filename << ":";
        }
        oss << line << ":" << column;
        return oss.str();
    }
};

} // namespace tacco

#endif // TACCO_LOCATION_HPP
