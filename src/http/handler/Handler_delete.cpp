#include <unistd.h>

#include <cstdio>

#include "../../utils/Log.hpp"
#include "Handler.hpp"
#include <sys/stat.h>

// DELETE Method implementation
void HttpHandler::handle_delete_request(
    const std::string& path, HttpResponse& response, const LocationBlock* location,
    const Connection* connection) {
    // Avoid unused parameter warning
    (void)connection;

    // Resolve the file path
    std::string file_path = resolve_file_path(path, location);

    // Validate delete operation
    validate_delete_operation(file_path);

    // Delete the file
    if (std::remove(file_path.c_str()) == 0) {
        Log::info("File deleted: " + file_path);
        response.set_status(OK);
        response.set_body("File deleted successfully");
    } else {
        throw HttpError(INTERNAL_SERVER_ERROR, "Failed to delete file");
    }
}

// Validate delete operation (check if file exists and is not a directory)
void HttpHandler::validate_delete_operation(const std::string& file_path) {
    struct stat path_stat;

    if (stat(file_path.c_str(), &path_stat) != 0) {
        throw HttpError(NOT_FOUND, "Resource not found");
    }

    if (!S_ISREG(path_stat.st_mode)) {
        throw HttpError(FORBIDDEN, "Cannot delete directories");
    }
}
