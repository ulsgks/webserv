#include "Handler.hpp"

#include "../../utils/Log.hpp"
#include "../common/Methods.hpp"
#include "../error/Error.hpp"

HttpHandler::HttpHandler() : server_block_(NULL) {
}

HttpHandler::~HttpHandler() {
}

// Main request handler - validates method and dispatches to appropriate handler
HttpResponse HttpHandler::handle_request(
    const HttpRequest& request, const ServerBlock& server_block, const Connection* connection) {
    HttpResponse response;

    // Store reference to server block for later use
    server_block_ = &server_block;

    HttpMethods::Method method = request.get_method();
    std::string path = request.get_path();
    const LocationBlock* location = NULL;

    try {
        location = server_block.match_location(path);
        if (!location) {
            throw HttpError(NOT_FOUND, "No matching location block");
        }

        // Check if this location has a redirect directive
        if (!location->redirect.empty()) {
            int status_code = (location->redirect_status_code > 0) ? location->redirect_status_code
                                                                   : DEFAULT_REDIRECT_STATUS;
            handle_redirect(response, location->redirect, status_code);
            return response;
        }

        // Special case for TRACE method - return 501 for security reasons
        // TRACE is commonly disabled server-wide for security (XST attacks)
        if (method == HttpMethods::TRACE) {
            throw HttpError(NOT_IMPLEMENTED, "TRACE method not implemented for security reasons");
        }

        // First check if we actually implement this method
        if (!HttpMethods::is_implemented(method)) {
            // For standard methods we don't implement (HEAD, PUT, OPTIONS, etc.)
            // we should return 405 Method Not Allowed, not 501
            // 501 is only for truly unknown methods (handled in request parsing)
            throw HttpError(METHOD_NOT_ALLOWED, "Method not allowed for this resource");
        }

        // Then check if this method is allowed on this specific location
        if (!location->is_allows_method(method)) {
            throw HttpError(METHOD_NOT_ALLOWED, "Method not allowed for this resource");
        }

        // Handle the request based on method
        switch (method) {
            case HttpMethods::GET:
                handle_get_request(request, path, response, location, connection);
                break;

            case HttpMethods::POST:
                handle_post_request(request, response, location, connection);
                break;

            case HttpMethods::DELETE:
                handle_delete_request(path, response, location, connection);
                break;

            default:  // This should not be reached
                throw HttpError(METHOD_NOT_ALLOWED, "Method not allowed for this resource");
        }
    } catch (const HttpError& e) {
        response = create_error_response(e, server_block, location);
    } catch (const std::exception& e) {
        // Wrap any standard exceptions in an HttpError
        HttpError server_error(INTERNAL_SERVER_ERROR, e.what());
        response = create_error_response(server_error, server_block, location);
    }
    return response;
}

// Handle redirects properly
void HttpHandler::handle_redirect(
    HttpResponse& response, const std::string& redirect_url, int status_code) {
    // Convert status code to HttpStatusCode enum
    HttpStatusCode http_status;
    switch (status_code) {
        case MOVED_PERMANENTLY:
            http_status = MOVED_PERMANENTLY;
            break;
        case FOUND:
            http_status = FOUND;
            break;
        case SEE_OTHER:
            http_status = SEE_OTHER;
            break;
        case TEMPORARY_REDIRECT:
            http_status = TEMPORARY_REDIRECT;
            break;
        case PERMANENT_REDIRECT:
            http_status = PERMANENT_REDIRECT;
            break;
        default:
            http_status = MOVED_PERMANENTLY;
            break;  // Default fallback
    }

    response.set_status(http_status);
    response.set_header(HttpHeaders::LOCATION, redirect_url);
    response.set_body(
        "<html><body>Redirected to <a href=\"" + redirect_url + "\">" + redirect_url +
        "</a></body></html>");
    response.set_header(HttpHeaders::CONTENT_TYPE, "text/html");
}

// NON-STANDARD FEATURE: Get stylesheet reference from server configuration
// Returns a link tag with the configured default_stylesheet or empty string if none configured
std::string HttpHandler::get_stylesheet_link() const {
    if (!server_block_ || server_block_->default_stylesheet.empty()) {
        return "";
    }
    return "<link rel=\"stylesheet\" href=\"" + server_block_->default_stylesheet + "\">";
}
