/**
 * @file parameters_interface.hpp
 * @brief Provides the FCU parameters capability contract.
 */

#pragma once

// clover2
#include <clover2_fcu/common/result.hpp>

// STL
#include <memory>
#include <string>
#include <variant>

namespace clover2_fcu::capability {

/// @brief FCU parameter value.
using parameter_value = std::variant<double, int64_t, bool, std::string>;

/**
 * @brief FCU parameters capability contract (synchronous get/set).
 *
 * Outcomes are reported via @ref clover2_fcu::result.
 */
class parameters_interface {
public:
    using shared_ptr = std::shared_ptr<parameters_interface>;

    static constexpr const char* name = "parameters";

    virtual ~parameters_interface() = default;

    /**
     * @brief Read an FCU parameter.
     * @param name parameter name.
     * @param out value output, untouched on failure.
     * @return result of the operation.
     */
    virtual result get(const std::string& name, parameter_value& out) = 0;

    /**
     * @brief Write an FCU parameter.
     * @param name parameter name.
     * @param value value to write.
     * @return result of the operation.
     */
    virtual result set(const std::string& name,
                       const parameter_value& value) = 0;
};

}  // namespace clover2_fcu::capability
