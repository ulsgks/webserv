#ifndef MIME_TYPES_HPP
#define MIME_TYPES_HPP

#include <string>

namespace MimeTypes {
    // Get MIME type based on file extension
    inline std::string get_type(const std::string& path) {
        std::string content_type = "application/octet-stream";  // Default MIME type
        size_t ext_pos = path.rfind('.');

        if (ext_pos != std::string::npos) {
            std::string ext = path.substr(ext_pos);

            // Web formats
            if (ext == ".html" || ext == ".htm") {
                content_type = "text/html";
            } else if (ext == ".css") {
                content_type = "text/css";
            } else if (ext == ".js") {
                content_type = "application/javascript";
            }
            // Image formats
            else if (ext == ".jpg" || ext == ".jpeg") {
                content_type = "image/jpeg";
            } else if (ext == ".png") {
                content_type = "image/png";
            } else if (ext == ".gif") {
                content_type = "image/gif";
            } else if (ext == ".ico") {
                content_type = "image/x-icon";
            } else if (ext == ".svg") {
                content_type = "image/svg+xml";
            }
            // Document formats
            else if (ext == ".txt") {
                content_type = "text/plain";
            } else if (ext == ".pdf") {
                content_type = "application/pdf";
            } else if (ext == ".json") {
                content_type = "application/json";
            } else if (ext == ".xml") {
                content_type = "application/xml";
            }
            // Archive formats
            else if (ext == ".zip") {
                content_type = "application/zip";
            }
            // Media formats
            else if (ext == ".mp3") {
                content_type = "audio/mpeg";
            } else if (ext == ".mp4") {
                content_type = "video/mp4";
            }
        }

        return content_type;
    }
}  // namespace MimeTypes

#endif  // MIME_TYPES_HPP
