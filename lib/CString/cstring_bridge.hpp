#ifndef CSTRING_BRIDGE_HPP
#define CSTRING_BRIDGE_HPP

#include <string>
#include <cstdlib>

extern "C" {
#include "CString/string_utils.h"
}

namespace cstring_bridge {

inline std::string toStdString(const String* s) {
    if (!s) return {};
    const size_t n = cstrlen(s);
    if (n == static_cast<size_t>(-1)) return {};
    const my_utf* utf = reinterpret_cast<const my_utf*>(s->data);
    if (!utf) return {};
    std::string out;
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        const uint8_t sz = utf[i].size;
        for (uint8_t j = 0; j < sz && j < 4; ++j) {
            const char c = static_cast<char>(utf[i].bytes[j]);
            if (c == '\0') break;
            out.push_back(c);
        }
    }
    return out;
}

inline bool equalsLit(const String* s, const char* lit) {
    return toStdString(s) == (lit ? lit : "");
}

inline String* makeString(const char* utf8) {
    String* s = static_cast<String*>(std::malloc(sizeof(String)));
    if (!s) return nullptr;
    s->length = 0;
    s->data = nullptr;
    initString(s, utf8 ? utf8 : "");
    return s;
}

inline String* makeString(const std::string& utf8) {
    return makeString(utf8.c_str());
}

inline void destroyString(String* s) {
    if (!s) return;
    freeString(s);
    std::free(s);
}

} // namespace cstring_bridge

#endif // CSTRING_BRIDGE_HPP

