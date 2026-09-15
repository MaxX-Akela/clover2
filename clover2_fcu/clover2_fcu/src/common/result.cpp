// clover2
#include <clover2_fcu/common/result.hpp>

// STL
#include <string>
#include <string_view>
#include <unordered_map>

namespace clover2_fcu {

std::string_view result::to_string() const noexcept {
    static const std::unordered_map<value, const char*> k_names = {
        {value::success, "success"},
        {value::rejected, "rejected"},
        {value::timeout, "timeout"},
        {value::interrupted, "interrupted"},
        {value::unsupported, "unsupported"},
    };

    const auto it = k_names.find(m_value);
    return it != k_names.end() ? it->second : "unknown";
}

std::optional<result> result::from_string(const std::string& name) {
    static const std::unordered_map<std::string, value> k_values = {
        {"success", value::success},
        {"rejected", value::rejected},
        {"timeout", value::timeout},
        {"interrupted", value::interrupted},
        {"unsupported", value::unsupported},
    };

    const auto it = k_values.find(name);
    if (it == k_values.end()) {
        return std::nullopt;
    }

    return result{it->second};
}

}  // namespace clover2_fcu
