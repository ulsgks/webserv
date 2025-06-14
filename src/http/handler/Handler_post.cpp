#include <unistd.h>  // For access() function

#include <cstring>
#include <fstream>

#include "../../utils/Log.hpp"
#include "../uri/Uri.hpp"
#include "Handler.hpp"
#include <sys/stat.h>

// File upload and parsing constants
static const size_t FILENAME_PREFIX_LENGTH = 10;   // Length of "filename=\""
static const mode_t DIRECTORY_PERMISSIONS = 0777;  // Default directory creation permissions

// POST Method implementation
void HttpHandler::handle_post_request(
    const HttpRequest& request, HttpResponse& response, const LocationBlock* location,
    const Connection* connection) {
    // Check if this is a CGI request
    if (is_cgi_request(request.get_path(), location)) {
        response = handle_cgi_request(request, request.get_path(), location, connection);
        return;
    }

    // Validate request and check size limits
    validate_post_request(request, location);

    // Get the content type
    std::string content_type = request.get_header(HttpHeaders::CONTENT_TYPE);

    // Handle based on content type
    if (content_type.find("multipart/form-data") != std::string::npos) {
        // Process multipart form data for file uploads
        handle_multipart_form_data(request, response, location);
    } else if (content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
        // Process URL-encoded form data
        handle_urlencoded_form(request, response, location);
    } else {
        // Unsupported content type - Let's be lenient and still accept form submissions
        // with missing or unknown content types (like NGINX does)
        handle_urlencoded_form(request, response, location);
    }
}

// Validate POST request and check size limits
void HttpHandler::validate_post_request(const HttpRequest& request, const LocationBlock* location) {
    // Check content length against location-specific limit
    std::string content_length = request.get_header(HttpHeaders::CONTENT_LENGTH);
    if (!content_length.empty()) {
        size_t length = std::strtoul(content_length.c_str(), NULL, 10);
        if (length > location->client_max_body_size) {
            throw HttpError(PAYLOAD_TOO_LARGE, "Content length exceeds maximum allowed size");
        }
    }
}

// Handler for multipart/form-data (file uploads)
void HttpHandler::handle_multipart_form_data(
    const HttpRequest& request, HttpResponse& response, const LocationBlock* location) {
    std::string boundary = extract_boundary(request);
    if (boundary.empty()) {
        throw HttpError(BAD_REQUEST, "Invalid multipart/form-data request");
    }

    // Process multipart form and upload files
    process_multipart_upload(request, boundary, response, location);
}

// Process multipart form data and handle file uploads
void HttpHandler::process_multipart_upload(
    const HttpRequest& request, const std::string& boundary, HttpResponse& response,
    const LocationBlock* location) {
    // First check if file uploads are properly configured
    std::string upload_dir = get_upload_directory(location);
    if (upload_dir.empty()) {
        // No upload directory configured - this is an error for file uploads
        throw HttpError(FORBIDDEN, "File uploads are not configured on this server");
    }

    std::string body = request.get_body();
    size_t pos = 0;
    bool file_uploaded = false;

    while ((pos = body.find("--" + boundary, pos)) != std::string::npos) {
        pos += boundary.length() + 2;
        size_t end_pos = body.find("--" + boundary, pos);
        if (end_pos == std::string::npos) {
            break;
        }

        std::string part = body.substr(pos, end_pos - pos);
        pos = end_pos;

        size_t header_end = part.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            continue;
        }

        std::string headers = part.substr(0, header_end);
        std::string content = part.substr(header_end + 4);

        if (handle_file_part(headers, content, location)) {
            file_uploaded = true;
        }
    }

    if (file_uploaded) {
        response.set_status(CREATED);
        std::string stylesheet_link =
            get_stylesheet_link();  // NON-STANDARD: Use configured stylesheet
        response.set_body(
            "<html><head><title>Uploaded successfully</title>" + stylesheet_link +
            "</head>"
            "<body><h1>File uploaded successfully</h1>"
            "<p>Your file has been uploaded to the server.</p>"
            "<button onclick=\"history.back()\">Go Back</button></body></html>");
    } else {
        throw HttpError(
            BAD_REQUEST, "No file found in the request or files could not be processed");
    }
}

// Handle a single file part in multipart form data
bool HttpHandler::handle_file_part(
    const std::string& headers, const std::string& content, const LocationBlock* location) {
    size_t filename_pos = headers.find("filename=\"");
    if (filename_pos == std::string::npos) {
        return false;
    }

    size_t filename_end = headers.find("\"", filename_pos + FILENAME_PREFIX_LENGTH);
    std::string filename = headers.substr(
        filename_pos + FILENAME_PREFIX_LENGTH,
        filename_end - (filename_pos + FILENAME_PREFIX_LENGTH));

    // Skip empty filenames (happens when user doesn't select a file)
    if (filename.empty()) {
        return false;
    }

    // Get the configured upload directory
    std::string upload_dir = get_upload_directory(location);
    if (upload_dir.empty()) {
        Log::warn("Received file upload but no upload_store configured");
        return false;
    }

    // Ensure the upload directory exists
    if (!ensure_upload_directory(upload_dir)) {
        throw HttpError(INTERNAL_SERVER_ERROR, "Failed to create upload directory");
    }

    std::string file_path = upload_dir + "/" + filename;

    // Check if file already exists
    if (access(file_path.c_str(), F_OK) == 0) {
        // File already exists, throw a conflict error
        Log::warn("Upload conflict: File already exists: " + filename);
        throw HttpError(CONFLICT, "File already exists: " + filename);
    }

    std::ofstream file(file_path.c_str(), std::ios::out | std::ios::binary);

    if (!file.is_open()) {
        throw HttpError(INTERNAL_SERVER_ERROR, "Failed to create file");
    }

    file.write(content.c_str(), content.size());
    file.close();

    Log::info("File uploaded: " + filename);

    return true;
}

// Handler for application/x-www-form-urlencoded (regular form submissions)
void HttpHandler::handle_urlencoded_form(
    const HttpRequest& request, HttpResponse& response, const LocationBlock* location) {
    // Parse and process the URL-encoded form data
    std::string body = request.get_body();
    std::map<std::string, std::string> form_data;

    // Parse form data - split by & and then by =
    size_t pos = 0;
    std::string token;
    while ((pos = body.find('&')) != std::string::npos) {
        token = body.substr(0, pos);
        parse_form_field(token, form_data);
        body.erase(0, pos + 1);
    }

    // Parse the last token
    if (!body.empty()) {
        parse_form_field(body, form_data);
    }

    // Get the upload directory (if configured)
    std::string upload_dir = get_upload_directory(location);
    bool stored = false;

    // Process the form data if storage is configured
    if (!upload_dir.empty()) {
        stored = process_form_data(form_data, location);
    }

    // Set appropriate status based on whether data was stored
    response.set_status(stored ? CREATED : OK);
    response.set_header(HttpHeaders::CONTENT_TYPE, "text/html");

    // Generate response body with received form data
    std::ostringstream response_body;
    std::string stylesheet_link = get_stylesheet_link();  // NON-STANDARD: Use configured stylesheet
    response_body << "<html><head><title>Form Submitted</title>" << stylesheet_link << "</head>"
                  << "<body><h1>Form data received successfully</h1>";

    if (!upload_dir.empty()) {
        if (!stored) {
            response_body
                << "<p class=\"error\">Warning: Data was received but could not be stored.</p>";
        }
    }

    response_body << "<p>The following data was submitted:</p><ul>";

    for (FormDataMapConstIt it = form_data.begin(); it != form_data.end(); ++it) {
        response_body << "<li><strong>" << it->first << ":</strong> " << it->second << "</li>";
    }

    response_body << "</ul><button onclick=\"history.back()\">Go Back</button></body></html>";
    response.set_body(response_body.str());
}

// Process form data based on location configuration
bool HttpHandler::process_form_data(
    const std::map<std::string, std::string>& form_data, const LocationBlock* location) {
    // Get the configured upload directory
    std::string upload_dir = get_upload_directory(location);

    // If no upload directory is configured, just return success
    if (upload_dir.empty()) {
        return true;
    }

    try {
        // Ensure the upload directory exists
        if (!ensure_upload_directory(upload_dir)) {
            Log::error("Failed to create upload directory for form data");
            return false;
        }

        // For demonstration, write form data to a file
        std::string file_path =
            upload_dir + "/form_submission_" + Log::to_string(time(NULL)) + ".txt";

        std::ofstream file(file_path.c_str());
        if (!file.is_open()) {
            Log::error("Failed to create form data file: " + file_path);
            return false;
        }

        // Write form data to file
        for (FormDataMapConstIt it = form_data.begin(); it != form_data.end(); ++it) {
            file << it->first << ": " << it->second << std::endl;
        }

        file.close();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Helper to parse a single form field (key=value)
void HttpHandler::parse_form_field(
    const std::string& field, std::map<std::string, std::string>& form_data) {
    size_t equal_pos = field.find('=');
    if (equal_pos != std::string::npos) {
        std::string key = field.substr(0, equal_pos);
        std::string value = field.substr(equal_pos + 1);

        // URL-decode both key and value
        form_data[Uri::decodeQueryParam(key)] = Uri::decodeQueryParam(value);
    } else {
        // Field with no value
        form_data[Uri::decodeQueryParam(field)] = "";
    }
}

//-----------------------------------------------------------------------------
// Utility functions
//-----------------------------------------------------------------------------

// Extract the boundary string from Content-Type header
std::string HttpHandler::extract_boundary(const HttpRequest& request) {
    std::string boundary;
    std::string content_type = request.get_header(HttpHeaders::CONTENT_TYPE);

    if (content_type.find("multipart/form-data") != std::string::npos) {
        size_t boundary_pos = content_type.find("boundary=");
        if (boundary_pos != std::string::npos) {
            boundary = content_type.substr(boundary_pos + 9);
        }
    }

    return boundary;
}

// Helper function to get the configured upload directory
std::string HttpHandler::get_upload_directory(const LocationBlock* location) const {
    // Check if location exists and has an upload_store configured
    if (!location || location->upload_store.empty()) {
        return "";  // No upload directory configured
    }

    return location->upload_store;
}

// Helper function to ensure upload directory exists
bool HttpHandler::ensure_upload_directory(const std::string& dir_path) const {
    if (dir_path.empty()) {
        return false;  // No directory to create
    }

    // Try to create the directory (does nothing if it already exists)
    if (mkdir(dir_path.c_str(), DIRECTORY_PERMISSIONS) != 0 && errno != EEXIST) {
        Log::error("Failed to create upload directory: " + dir_path + " (" + strerror(errno) + ")");
        return false;
    }

    return true;
}
