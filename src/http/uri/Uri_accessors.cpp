#include "../../utils/Types.hpp"
#include "Uri.hpp"

// Query parameter methods
std::string Uri::get_query_param(const std::string& param_name) const {
    QueryParamMapConstIt it = query_params_.find(param_name);
    if (it != query_params_.end()) {
        return it->second;
    }
    return "";
}

bool Uri::has_query_param(const std::string& param_name) const {
    return query_params_.find(param_name) != query_params_.end();
}

// String representation
std::string Uri::to_string() const {
    std::string result;

    if (absolute_) {
        result = scheme_ + ":// ";
        result += host_;

        // Add port if non-standard
        if ((scheme_ == "http" && port_ != Uri::HTTP_DEFAULT_PORT) ||
            (scheme_ == "https" && port_ != Uri::HTTPS_DEFAULT_PORT)) {
            result += ":" + Uri::to_string_uri(port_);
        }
    }

    result += path_;

    if (!query_string_.empty()) {
        result += "?" + query_string_;
    }

    return result;
}

// Accessor methods
const std::string& Uri::get_path() const {
    return path_;
}

const std::string& Uri::get_query_string() const {
    return query_string_;
}

const std::string& Uri::get_scheme() const {
    return scheme_;
}

const std::string& Uri::get_host() const {
    return host_;
}

int Uri::get_port() const {
    return port_;
}

bool Uri::is_absolute() const {
    return absolute_;
}

bool Uri::is_valid() const {
    return valid_;
}
