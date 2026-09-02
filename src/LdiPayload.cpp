#include "LdiPayload.h"

#include <cstdio>
#include <sstream>

namespace DcvLdiExtension {

namespace {

std::string EscapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

}  // namespace

std::string LdiPayload::ToJson() const {
    std::ostringstream out;
    out << "{"
        << "\"schemaVersion\":\"" << EscapeJson(schemaVersion) << "\","
        << "\"hostname\":\"" << EscapeJson(hostname) << "\","
        << "\"ipv4\":\"" << EscapeJson(ipv4) << "\","
        << "\"ipv6\":null,"
        << "\"adapterType\":\"" << EscapeJson(adapterType) << "\","
        << "\"lastUpdatedUtc\":\"" << EscapeJson(lastUpdatedUtc) << "\""
        << "}";
    return out.str();
}

}  // namespace DcvLdiExtension
