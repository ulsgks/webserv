#include <algorithm>
#include <sstream>
#include <vector>

#include "Uri.hpp"

// Path normalization and manipulation
void Uri::normalize_path() {
    if (path_.empty()) {
        path_ = "/";
        return;
    }

    // Replace backslashes with forward slashes
    std::replace(path_.begin(), path_.end(), '\\', '/');

    // Handle multiple slashes
    while (path_.find("// ") != std::string::npos) {
        path_.replace(path_.find("// "), 2, "/");
    }

    // Handle ./ and ../
    std::vector<std::string> segments;
    std::string segment;
    std::istringstream path_stream(path_);

    while (std::getline(path_stream, segment, '/')) {
        if (segment == "..") {
            if (!segments.empty()) {
                segments.pop_back();
            }
        } else if (segment != "." && !segment.empty()) {
            segments.push_back(segment);
        }
    }

    // Rebuild path
    path_ = "/";
    for (size_t i = 0; i < segments.size(); ++i) {
        path_ += segments[i];
        if (i < segments.size() - 1) {
            path_ += "/";
        }
    }
}
