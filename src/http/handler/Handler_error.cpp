#include <fstream>
#include <sstream>

#include "../../utils/Log.hpp"
#include "Handler.hpp"

// Create error response with custom error page if available
HttpResponse HttpHandler::create_error_response(
    const HttpError& e, const ServerBlock& server_block, const LocationBlock* location) {
    int status_code = e.get_status_code();

    // Build the error response
    HttpResponse response;
    bool custom_page_found = find_custom_error_page(response, status_code, server_block, location);

    if (!custom_page_found) {
        response = HttpResponse::build_default_error_response(e);
    }

    // Special case for 405 Method Not Allowed - always add the Allow header
    if (status_code == METHOD_NOT_ALLOWED && location) {
        // Build the Allow header with implemented methods that are allowed on this location
        std::string allowed_methods;
        bool first = true;

        // Check each implemented method to see if it's allowed
        if (location->is_allows_method(HttpMethods::GET)) {
            if (!first)
                allowed_methods += ", ";
            allowed_methods += "GET";
            first = false;
        }
        if (location->is_allows_method(HttpMethods::POST)) {
            if (!first)
                allowed_methods += ", ";
            allowed_methods += "POST";
            first = false;
        }
        if (location->is_allows_method(HttpMethods::DELETE)) {
            if (!first)
                allowed_methods += ", ";
            allowed_methods += "DELETE";
            first = false;
        }

        // If no methods are allowed (shouldn't happen), at least include GET
        if (allowed_methods.empty()) {
            allowed_methods = "GET";
        }

        response.set_header(HttpHeaders::ALLOW, allowed_methods);
    }
    // If we don't have location info but still got 405, add a generic Allow header
    else if (status_code == METHOD_NOT_ALLOWED && !location) {
        response.set_header(HttpHeaders::ALLOW, "GET, POST, DELETE");
    }

    return response;
}

bool HttpHandler::find_custom_error_page(
    HttpResponse& response, int status_code, const ServerBlock& server_block,
    const LocationBlock* location) {
    // First check location-specific error pages if provided
    if (location) {
        ErrorPageMapConstIt error_page = location->error_pages.find(status_code);

        if (error_page != location->error_pages.end()) {
            std::string resolved_path = resolve_error_page_path(error_page->second, location);
            if (try_load_error_page(response, resolved_path, status_code)) {
                return true;
            }
        }
    }

    // Then check server-level error pages
    ErrorPageMapConstIt error_page = server_block.error_pages.find(status_code);

    if (error_page != server_block.error_pages.end()) {
        std::string resolved_path = server_block.root + error_page->second;
        return try_load_error_page(response, resolved_path, status_code);
    }

    // No custom error page found
    return false;
}

// Helper method to resolve error page path based on context
std::string HttpHandler::resolve_error_page_path(
    const std::string& error_page_path, const LocationBlock* location) {
    std::string root_dir = location ? get_root_directory(location) : server_block_->root;

    // For server-level or root location error pages
    return root_dir + error_page_path;
}

// Modified to directly update the response parameter
bool HttpHandler::try_load_error_page(
    HttpResponse& response, const std::string& resolved_path, int status_code) {
    try {
        std::ifstream file(resolved_path.c_str());
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();

            response.set_status((HttpStatusCode)status_code);
            response.set_header(HttpHeaders::CONTENT_TYPE, "text/html");
            response.set_body(buffer.str());
            return true;
        }
    } catch (const std::exception& e) {
        Log::error("Error loading error page: " + std::string(e.what()));
    }
    return false;
}
