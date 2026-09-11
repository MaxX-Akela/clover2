// clover2
#include <clover2_fcu/capability/all.hpp>

// STL
#include <type_traits>

namespace clover2_fcu::capability {

namespace detail {

constexpr bool names_equal(const char* a, const char* b) {
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return false;
        }

        ++a;
        ++b;
    }

    return *a == *b;
}

}  // namespace detail

static_assert(
    []<typename... Ts>(std::type_identity<std::tuple<Ts...>>) {
        return detail::names_unique<Ts...>();
    }(std::type_identity<all>{}),
    "Capability contract names must be unique");

const std::vector<std::string>& all_names() {
    static const std::vector<std::string> names =
        []<typename... Ts>(std::type_identity<std::tuple<Ts...>>) {
            std::vector<std::string> out;
            (..., out.emplace_back(Ts::name));
            return out;
        }(std::type_identity<all>{});

    return names;
}

}  // namespace clover2_fcu::capability
