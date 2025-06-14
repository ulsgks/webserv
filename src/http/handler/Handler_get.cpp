#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <sstream>

#include "../../utils/Log.hpp"
#include "Handler.hpp"
#include <sys/stat.h>

void HttpHandler::handle_get_request(
    const HttpRequest& request, const std::string& path, HttpResponse& response,
    const LocationBlock* location, const Connection* connection) {
    // Check if this is a CGI request
    if (is_cgi_request(path, location)) {
        response = handle_cgi_request(request, path, location, connection);
        return;
    }

    // Security check - validate the path before proceeding
    validate_file_access(path);

    // Resolve file path
    std::string file_path = resolve_file_path(path, location);

    // Check if path exists
    struct stat path_stat;
    if (stat(file_path.c_str(), &path_stat) != 0) {
        throw HttpError(NOT_FOUND, "Resource not found: " + path);
    }

    if (S_ISDIR(path_stat.st_mode)) {
        // Handle directory request
        handle_directory_request(path, file_path, response, location);
    } else {
        // Handle file request
        handle_file_request(file_path, response);
    }
}

// Handle directory requests (index files or directory listings)
void HttpHandler::handle_directory_request(
    const std::string& path, const std::string& file_path, HttpResponse& response,
    const LocationBlock* location) {
    // Try to serve index file if configured
    if (!location->index.empty()) {
        if (serve_index_file(file_path, location->index, response)) {
            return;
        }
    }

    // If no index or index not found, check if autoindex is enabled
    if (location->autoindex) {
        generate_directory_listing(path, file_path, response);
    } else {
        throw HttpError(FORBIDDEN, "Directory listing not allowed");
    }
}

// Attempt to serve an index file, returns true if successful
bool HttpHandler::serve_index_file(
    const std::string& dir_path, const std::string& index_name, HttpResponse& response) {
    std::string index_path = dir_path;
    if (index_path[index_path.size() - 1] != '/') {
        index_path += "/";
    }
    index_path += index_name;

    if (access(index_path.c_str(), F_OK) != -1) {
        // Index file exists, serve it
        std::ifstream index_file(index_path.c_str());
        if (index_file.is_open()) {
            std::stringstream buffer;
            buffer << index_file.rdbuf();
            index_file.close();

            response.set_status(OK);
            response.set_header(HttpHeaders::CONTENT_TYPE, "text/html");
            response.set_body(buffer.str());
            return true;
        }
    }
    return false;
}

// Generate HTML directory listing
void HttpHandler::generate_directory_listing(
    const std::string& path, const std::string& file_path, HttpResponse& response) {
    DIR* dir;
    struct dirent* ent;

    if ((dir = opendir(file_path.c_str())) != NULL) {
        std::ostringstream buff;
        buff << "<html><head><title>Directory listing for " << path << "</title>";
        buff << "<meta charset=\"UTF-8\">";  // Specify UTF-8 encoding
        buff << get_stylesheet_link();       // NON-STANDARD: Use configured stylesheet
        buff << "</head>";
        buff << "<body><h1>Directory listing for " << path << "</h1><hr>";

        // Add a "Back" button to navigate to the parent directory
        std::string parent_path = path;
        if (parent_path != "/") {
            size_t last_slash = parent_path.find_last_of('/');
            if (last_slash != std::string::npos) {
                parent_path = parent_path.substr(0, last_slash);
                if (parent_path.empty()) {
                    parent_path = "/";
                }
            }
        }

        buff << "<ul>";

        // Separate directories and files
        std::vector<std::string> directories;
        std::vector<std::string> files;

        while ((ent = readdir(dir)) != NULL) {
            std::string name = ent->d_name;

            // Skip "." and ".."
            if (name == "." || name == "..") {
                continue;
            }

            // Skip hidden files in directory listings for security
            if (name[0] == '.') {
                continue;
            }

            // Check if it's a directory
            std::string full_path = file_path + "/" + name;
            struct stat path_stat;
            if (stat(full_path.c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
                directories.push_back(name + "/");  // Add "/" to directories
            } else {
                files.push_back(name);
            }
        }

        closedir(dir);

        // Check if the directory is empty
        if (directories.empty() && files.empty()) {
            buff << "<li><em>Directory is empty</em></li>";
        } else {
            // Sort directories and files
            std::sort(directories.begin(), directories.end());
            std::sort(files.begin(), files.end());

            // Add directories to the listing
            for (StringVectorConstIt it = directories.begin(); it != directories.end(); ++it) {
                buff << "<li><a href=\"" << path;
                if (path[path.size() - 1] != '/')
                    buff << "/";
                buff << *it << "\">" << *it << "</a></li>";
            }

            // Add files to the listing
            for (StringVectorConstIt it = files.begin(); it != files.end(); ++it) {
                buff << "<li><a href=\"" << path;
                if (path[path.size() - 1] != '/')
                    buff << "/";
                buff << *it << "\">" << *it << "</a></li>";
            }
        }

        buff << "</ul>";
        buff << "<form action=\"" << parent_path << "\" method=\"get\">";
        buff << "<button type=\"submit\">Back to Parent Directory ↲</button>";
        buff << "</form>";
        buff << "<hr></body></html>";

        response.set_status(OK);
        response.set_header(HttpHeaders::CONTENT_TYPE, "text/html");
        response.set_body(buff.str());
    } else {
        throw HttpError(INTERNAL_SERVER_ERROR, "Failed to open directory");
    }
}
