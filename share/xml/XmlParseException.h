#pragma once

#include <stdexcept>
#include <string>

class XmlParseException final : public std::runtime_error {
public:
    explicit XmlParseException(const std::string& message)
        : std::runtime_error(message) {}
};
