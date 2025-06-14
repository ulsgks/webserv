#!/bin/bash

# This CGI script sleeps for longer than the timeout to demonstrate a 504 error

# Sleep for 35 seconds (longer than the 30 second timeout)
sleep 35

# This will never be reached due to timeout
echo "Content-Type: text/html"
echo ""
echo "You shouldn't see this!"