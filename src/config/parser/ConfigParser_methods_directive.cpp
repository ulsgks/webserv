// src/config/parser/ParserMethodsDirective.cpp
#include <algorithm>

#include "../../http/common/Methods.hpp"
#include "../../utils/Log.hpp"
#include "ConfigParser.hpp"

void ConfigParser::parse_methods_directive(
    LocationBlock& location, const std::vector<std::string>& values,
    const ConfigToken& directive_token) {
    location.allowed_methods.clear();

    if (values.empty()) {
        syntax_error("methods directive requires at least one value", directive_token);
    }
    for (size_t i = 0; i < values.size(); ++i) {
        std::string method_str = values[i];
        std::transform(method_str.begin(), method_str.end(), method_str.begin(), ::toupper);

        // Use our centralized method validation
        if (!HttpMethods::is_standard_method(method_str)) {
            syntax_error("Invalid HTTP method: " + values[i], directive_token);
        }

        // Add the method enum to allowed methods
        HttpMethods::Method method = HttpMethods::from_string(method_str);

        // Check if we actually implement this method
        if (!HttpMethods::is_implemented(method)) {
            // Warn but allow - the server will return 405 at runtime
            Log::warn("Method " + method_str + " is configured but not implemented by the server");
        }

        location.allowed_methods.push_back(method);
    }
}
