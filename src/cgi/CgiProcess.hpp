#ifndef CGI_PROCESS_HPP
#define CGI_PROCESS_HPP

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "../http/error/Error.hpp"
#include "../utils/Log.hpp"
#include <sys/wait.h>

/**
 * Handles CGI process creation and execution.
 *
 * This namespace is responsible for:
 * - Creating child processes
 * - Setting up pipes for I/O
 * - Managing process lifecycle
 * - Handling timeouts
 */
namespace CgiProcess {
    // CGI execution constants
    static const int CGI_TIMEOUT_SECONDS = 5;    // CGI process timeout in seconds
    static const size_t CGI_BUFFER_SIZE = 8192;  // I/O buffer size for CGI communication
    static const int POLL_INTERVAL_MICROSECONDS =
        100000;  // Polling interval in microseconds (100ms)

    // Helper functions
    inline void create_pipes(int stdin_pipe[2], int stdout_pipe[2]);
    inline void execute_child_process(
        const std::string& interpreter, const std::string& absolute_script_path, char** envp,
        int stdin_pipe[2], int stdout_pipe[2]);
    inline std::string get_absolute_path(const std::string& script_path);
    inline char** vector_to_envp(const std::vector<std::string>& env_vector);
    inline void free_envp(char** envp);
    inline std::string get_script_filename(const std::string& path);
    inline std::string get_script_directory(const std::string& path);

    // Non-blocking CGI execution for server integration
    inline bool start_execution(
        const std::string& interpreter, const std::string& script_path,
        const std::vector<std::string>& env_vector, const std::string& request_body, pid_t& out_pid,
        int& out_stdout_fd, int& out_stdin_fd);

    // Implementation

    // Non-blocking CGI execution starter
    inline bool start_execution(
        const std::string& interpreter, const std::string& script_path,
        const std::vector<std::string>& env_vector, const std::string& request_body, pid_t& out_pid,
        int& out_stdout_fd, int& out_stdin_fd) {
        char** envp = vector_to_envp(env_vector);

        try {
            std::string absolute_script_path = get_absolute_path(script_path);

            // Create pipes for communication
            int stdin_pipe[2];
            int stdout_pipe[2];
            create_pipes(stdin_pipe, stdout_pipe);

            pid_t pid = fork();
            if (pid == -1) {
                close(stdin_pipe[0]);
                close(stdin_pipe[1]);
                close(stdout_pipe[0]);
                close(stdout_pipe[1]);
                free_envp(envp);
                return false;
            }

            if (pid == 0) {
                // Child process
                execute_child_process(
                    interpreter, absolute_script_path, envp, stdin_pipe, stdout_pipe);
            }

            // Parent process - setup for non-blocking monitoring
            close(stdin_pipe[0]);   // Close read end of stdin pipe
            close(stdout_pipe[1]);  // Close write end of stdout pipe

            // Set stdout pipe to non-blocking mode
            fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);

            // Set stdin pipe to non-blocking mode for potential request body sending
            fcntl(stdin_pipe[1], F_SETFL, O_NONBLOCK);

            // Return file descriptors for monitoring
            // CgiManager will handle request body sending through poll() if needed
            out_pid = pid;
            out_stdout_fd = stdout_pipe[0];
            out_stdin_fd =
                request_body.empty() ? -1 : stdin_pipe[1];  // Keep stdin open if we have body data

            // Close stdin immediately if no request body (common case)
            if (request_body.empty()) {
                close(stdin_pipe[1]);
            }

            free_envp(envp);
            return true;

        } catch (...) {
            free_envp(envp);
            return false;
        }
    }

    inline std::string get_absolute_path(const std::string& script_path) {
        if (script_path[0] == '/') {
            return script_path;
        }

        char* cwd = getcwd(NULL, 0);
        if (cwd) {
            std::string absolute_path = std::string(cwd) + "/" + script_path;
            free(cwd);
            return absolute_path;
        }
        return script_path;
    }

    inline void create_pipes(int stdin_pipe[2], int stdout_pipe[2]) {
        if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
            throw HttpError(INTERNAL_SERVER_ERROR, "Failed to create pipes for CGI");
        }

        // Set FD_CLOEXEC on all pipe file descriptors to prevent inheritance by CGI child
        fcntl(stdin_pipe[0], F_SETFD, FD_CLOEXEC);
        fcntl(stdin_pipe[1], F_SETFD, FD_CLOEXEC);
        fcntl(stdout_pipe[0], F_SETFD, FD_CLOEXEC);
        fcntl(stdout_pipe[1], F_SETFD, FD_CLOEXEC);
    }

    inline void execute_child_process(
        const std::string& interpreter, const std::string& absolute_script_path, char** envp,
        int stdin_pipe[2], int stdout_pipe[2]) {
        // Redirect stdin and stdout
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);

        // Close all pipe ends
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);

        // FD_CLOEXEC flags on pipes ensure server sockets and other file descriptors
        // are automatically closed when execve() is called, eliminating the need
        // for manual cleanup loop. This is cleaner and more reliable.

        // Change to script directory
        std::string script_dir = get_script_directory(absolute_script_path);
        if (chdir(script_dir.c_str()) == -1) {
            std::cerr << "CGI: Failed to change directory to " << script_dir << std::endl;
            exit(1);
        }

        // Prepare arguments for execve
        const char* argv[3];

        if (interpreter.empty()) {
            // Direct execution for .cgi files
            argv[0] = absolute_script_path.c_str();
            argv[1] = NULL;
            execve(absolute_script_path.c_str(), const_cast<char**>(argv), envp);
        } else {
            // Interpreted execution for other scripts
            argv[0] = interpreter.c_str();

            // Store filename in persistent variable to avoid dangling pointer
            std::string script_filename = get_script_filename(absolute_script_path);
            argv[1] = script_filename.c_str();
            argv[2] = NULL;

            execve(interpreter.c_str(), const_cast<char**>(argv), envp);
        }

        // If we get here, execve failed
        std::cerr << "CGI: execve failed: " << strerror(errno) << std::endl;
        exit(1);
    }

    inline char** vector_to_envp(const std::vector<std::string>& env_vector) {
        // Allocate array of char pointers (+1 for NULL terminator)
        char** envp = new char*[env_vector.size() + 1];

        // Copy each environment string
        for (size_t i = 0; i < env_vector.size(); ++i) {
            envp[i] = new char[env_vector[i].length() + 1];
            std::strcpy(envp[i], env_vector[i].c_str());
        }

        // NULL terminate the array
        envp[env_vector.size()] = NULL;

        return envp;
    }

    inline void free_envp(char** envp) {
        if (!envp)
            return;

        // Free each string
        for (int i = 0; envp[i] != NULL; ++i) {
            delete[] envp[i];
        }

        // Free the array itself
        delete[] envp;
    }

    inline std::string get_script_filename(const std::string& path) {
        size_t last_slash = path.find_last_of('/');
        if (last_slash != std::string::npos) {
            return path.substr(last_slash + 1);
        }
        return path;
    }

    inline std::string get_script_directory(const std::string& path) {
        size_t last_slash = path.find_last_of('/');
        if (last_slash != std::string::npos) {
            return path.substr(0, last_slash);
        }
        return ".";
    }

}  // namespace CgiProcess

#endif  // CGI_PROCESS_HPP
