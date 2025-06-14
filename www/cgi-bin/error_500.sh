#!/bin/bash

# This CGI script intentionally crashes to demonstrate a 500 error

# Output headers first
echo "Content-Type: text/html"
echo ""

# Start outputting HTML
echo "<!DOCTYPE html>"
echo "<html><head><title>About to crash...</title></head><body>"
echo "<h1>This script is about to crash!</h1>"

# Intentionally cause an error
# Exit with non-zero status to indicate failure
exit 1