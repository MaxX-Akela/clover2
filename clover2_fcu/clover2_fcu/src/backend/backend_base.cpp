// clover2
#include <clover2_fcu/backend/backend_base.hpp>

namespace clover2_fcu::backend {

std::vector<std::string> backend_base::capabilities() const {
    std::vector<std::string> names;
    names.reserve(m_builders.size());

    for (const auto& [name, builder] : m_builders) {
        names.push_back(name);
    }

    return names;
}

void backend_base::initialize(context& ctx, const YAML::Node& params) {
    if (m_loaded) {
        return;
    }

    do_initialize(ctx, params);
    m_loaded = true;
}

std::shared_ptr<void> backend_base::create(const std::string& capability,
                                           context& ctx,
                                           const YAML::Node& params) const {
    if (!m_loaded) {
        throw exception(
            "factory '{}' is not initialized (initialize() must be called "
            "before create())",
            protocol());
    }

    const auto it = m_builders.find(capability);
    if (it == m_builders.end()) {
        throw exception("factory '{}' does not serve capability '{}'",
                        protocol(), capability);
    }
    return it->second(ctx, params);
}

}  // namespace clover2_fcu::backend
