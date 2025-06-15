# -----------------------------------------------------------------------------
# Webserver Makefile - Updated for subdirectory support
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Compiler and Standard Flags
# -----------------------------------------------------------------------------
CXX             = c++
CXXFLAGS        = -Wall -Wextra -Werror -std=c++98 -pedantic
# Safety Flags
CXXFLAGS        += -Wshadow -Wnull-dereference -Wcast-align -Wcast-qual
# Best Practices Flags
CXXFLAGS        += -Wnon-virtual-dtor -Woverloaded-virtual -Wunreachable-code
# Optimization Flags
CXXFLAGS        += -O3
RM              = rm -rf
NAME            = webserv
DEBUG_NAME      = webserv_debug

# -----------------------------------------------------------------------------
# Colors for Terminal Output
# -----------------------------------------------------------------------------
RED             = \033[0;31m
YELLOW          = \033[0;33m
GREEN           = \033[0;32m
RESET           = \033[0m

# -----------------------------------------------------------------------------
# Debug Configuration
# -----------------------------------------------------------------------------
DEBUG_FLAGS     = -g3 -O0
SANITIZE_FLAGS  = -fsanitize=address -fsanitize=undefined# -fsanitize=leak

# -----------------------------------------------------------------------------
# Directory Structure
# -----------------------------------------------------------------------------
BUILD_DIR       = build
DEBUG_BUILD_DIR = build_debug
SRC_DIR         = src
INCLUDE_DIR     = includes

# -----------------------------------------------------------------------------
# Source Files - Using recursive search for subdirectories
# -----------------------------------------------------------------------------
# Find all .cpp files recursively in src directory
SRC_FILES       = $(shell find $(SRC_DIR) -name "*.cpp")

# Headers - Find all .hpp files recursively
HEADERS         = $(shell find $(SRC_DIR) -name "*.hpp") \
                  $(wildcard $(INCLUDE_DIR)/*.hpp)

# -----------------------------------------------------------------------------
# Object Files and Dependencies
# -----------------------------------------------------------------------------
# Standard build objects - preserves directory structure
OBJ             = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC_FILES))
DEP             = $(OBJ:.o=.d)

# Debug build objects - preserves directory structure
DEBUG_OBJ       = $(patsubst $(SRC_DIR)/%.cpp,$(DEBUG_BUILD_DIR)/%.o,$(SRC_FILES))
DEBUG_DEP       = $(DEBUG_OBJ:.o=.d)

# -----------------------------------------------------------------------------
# Standard Build Rules
# -----------------------------------------------------------------------------
all:            $(NAME)

$(NAME):        $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@
	@echo -e "$(GREEN)Build complete: $(NAME)$(RESET)"

# Compile rule creates directories automatically with $(dir $@)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@ -I$(INCLUDE_DIR) -I$(SRC_DIR)

# -----------------------------------------------------------------------------
# Debug Build Rules
# -----------------------------------------------------------------------------
debug:          $(DEBUG_NAME)

$(DEBUG_NAME):  $(DEBUG_OBJ)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(SANITIZE_FLAGS) $(DEBUG_OBJ) -o $@
	@echo -e "$(GREEN)Debug build complete: $(DEBUG_NAME)$(RESET)"

# Debug compile rule creates directories automatically
$(DEBUG_BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(SANITIZE_FLAGS) -MMD -MP -c $< -o $@ -I$(INCLUDE_DIR) -I$(SRC_DIR)

run-debug:      debug
	@echo -e "$(RED)WARNING: Running in debug mode with sanitizers enabled: this will impact performance.$(RESET)"
	./$(DEBUG_NAME)

# -----------------------------------------------------------------------------
# Cleanup Rules
# -----------------------------------------------------------------------------
clean:
	$(RM) $(BUILD_DIR)
	$(RM) $(DEBUG_BUILD_DIR)
	@echo -e "$(YELLOW)Cleaned build directories$(RESET)"

fclean:         clean
	$(RM) $(NAME)
	$(RM) $(DEBUG_NAME)
	@echo -e "$(RED)Removed executables$(RESET)"

# -----------------------------------------------------------------------------
# Rebuild Rules
# -----------------------------------------------------------------------------
re:             fclean all

re-debug:       fclean debug

# -----------------------------------------------------------------------------
# Run Rules
# -----------------------------------------------------------------------------
run:            all
	./$(NAME)

# -----------------------------------------------------------------------------
# Include Dependencies
# -----------------------------------------------------------------------------
-include $(DEP)
-include $(DEBUG_DEP)

.PHONY:         all clean fclean re run debug run-debug re-debug

.SILENT: