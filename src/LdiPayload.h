#pragma once

#include <string>

namespace DcvLdiExtension {

// Mirrors DcvLdiExtension.LdiPayload from the C# implementation. `ipv6` is
// intentionally not a field here: the C# collector never populates it either
// (it's always serialized as JSON null), so ToJson() emits the literal null
// directly rather than modeling an always-empty field.
struct LdiPayload {
    std::string schemaVersion = "1.0.0";
    std::string hostname;
    std::string ipv4 = "0.0.0.0";
    std::string adapterType = "UNKNOWN";
    std::string lastUpdatedUtc;

    std::string ToJson() const;
};

}  // namespace DcvLdiExtension
