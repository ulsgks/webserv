#include <fstream>

#include "../../utils/Log.hpp"
#include "../common/MimeTypes.hpp"
#include "Handler.hpp"

// Handle static file requests - main entry point for file serving
void HttpHandler::handle_file_request(const std::string& file_path, HttpResponse& response) {
    std::ifstream file(file_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        throw HttpError(NOT_FOUND, "File not found");
    }

    // Read the entire file
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Set appropriate content type
    std::string content_type = MimeTypes::get_type(file_path);

    response.set_status(OK);
    response.set_header(HttpHeaders::CONTENT_TYPE, content_type);
    response.set_body(content);
}

// Main file path resolution function
std::string HttpHandler::resolve_file_path(
    const std::string& request_path, const LocationBlock* location) const {
    // Handle root/empty path specially
    if (request_path.empty() || request_path == "/") {
        std::string root_dir = get_root_directory(location);
        return root_dir + "/" + location->index;
    }

    // Get the root directory for this location
    std::string root_dir = get_root_directory(location);

    // Location matching is still done with the encoded path (already happened)
    // But now decode the path when building the file path
    std::string decoded_path = Uri::decode(request_path);

    // Build the file path based on the type of match
    if (is_exact_match_for_location(request_path, location)) {
        return build_exact_match_path(root_dir, location);
    } else if (is_prefix_match_for_location(request_path, location)) {
        // Use the decoded path when building the file path
        return build_prefix_match_path(root_dir, decoded_path, location);
    } else {
        // For non-matches, use the decoded path with server root
        return root_dir + decoded_path;
    }
}

// Helper functions for resolve_file_path
bool HttpHandler::is_exact_match_for_location(
    const std::string& request_path, const LocationBlock* location) const {
    return location->exact_match && request_path == location->path;
}

bool HttpHandler::is_prefix_match_for_location(
    const std::string& request_path, const LocationBlock* location) const {
    // Root location is a special case - it matches everything
    if (location->path == "/") {
        return true;
    }

    // Check if request path starts with location path
    if (request_path.find(location->path) != 0) {
        return false;
    }

    // It's a prefix match if paths are identical or next char is a slash
    return (
        request_path.length() == location->path.length() ||
        request_path[location->path.length()] == '/');
}

std::string HttpHandler::get_root_directory(const LocationBlock* location) const {
    // Use location root if available, otherwise fall back to server root
    if (!location->root.empty()) {
        return location->root;
    }

    if (!server_block_->root.empty()) {
        return server_block_->root;
    }

    throw HttpError(INTERNAL_SERVER_ERROR, "No root directory configured for this path");
}

std::string HttpHandler::build_exact_match_path(
    const std::string& root_dir, const LocationBlock* location) const {
    return root_dir + "/" + location->index;
}

std::string HttpHandler::build_prefix_match_path(
    const std::string& root_dir, const std::string& request_path,
    const LocationBlock* location) const {
    // Special case for root location
    if (location->path == "/") {
        return root_dir + request_path;
    }

    // Extract the part of the path after the location prefix
    std::string location_path = location->path;
    std::string decoded_location_path = Uri::decode(location_path);

    std::string remaining_path;
    if (request_path.length() > decoded_location_path.length()) {
        remaining_path = request_path.substr(decoded_location_path.length());
    }

    // Ensure remaining path starts with a slash if not empty
    if (!remaining_path.empty() && remaining_path[0] != '/') {
        remaining_path = "/" + remaining_path;
    }

    // If the request path exactly matches the location path
    // and there's no remaining path, check if it's a file (has extension)
    if (request_path == location_path && remaining_path.empty()) {
        // Check if the path has a file extension (contains a dot after the last slash)
        size_t last_slash = location_path.find_last_of('/');
        size_t last_dot = location_path.find_last_of('.');

        if (last_dot != std::string::npos &&
            (last_slash == std::string::npos || last_dot > last_slash)) {
            // This is likely a file, extract the file name
            std::string file_name = location_path;
            if (last_slash != std::string::npos) {
                file_name = file_name.substr(last_slash + 1);
            }
            return root_dir + "/" + file_name;
        }
    }

    // If location has its own root, use that with the remaining path
    if (!location->root.empty()) {
        return location->root + remaining_path;
    }

    // If no location-specific root, include the location path in the server root path
    return root_dir + location_path + remaining_path;
}
