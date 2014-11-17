#ifndef SQUID_ANALYZE_TYPES_HPP
#define SQUID_ANALYZE_TYPES_HPP

#include <cstdint>

struct log_line {
   log_line();
   uint32_t z, x, y, ip_addr;
};

inline bool operator==(const log_line &a, const log_line &b) {
   return ((a.z == b.z) && (a.x == b.x) && (a.y == b.y) && (a.ip_addr == b.ip_addr));
}

inline bool operator!=(const log_line &a, const log_line &b) {
   return !operator==(a, b);
}

inline bool operator<(const log_line &a, const log_line &b) {
   return ((a.z < b.z) ||
           (a.z == b.z) && ((a.x < b.x) ||
                            (a.x == b.x) && ((a.y < b.y) ||
                                             (a.y == b.y) && (a.ip_addr < b.ip_addr))));
}

#endif /* SQUID_ANALYZE_TYPES_HPP */
