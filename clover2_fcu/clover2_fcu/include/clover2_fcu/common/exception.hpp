#pragma once

// STL
#include <format>

namespace clover2_fcu {

class exception : public std::exception {
public:
    template <typename... Args>
    exception(std::format_string<Args...> fmt, Args&&... args) noexcept
        : m_message(std::format(fmt, std::forward<Args>(args)...)) {}

    const char* what() const noexcept override { return m_message.c_str(); }
    const std::string& message() const noexcept { return m_message; }

private:
    std::string m_message;
};

}  // namespace clover2_fcu
