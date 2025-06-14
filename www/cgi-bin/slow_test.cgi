#!/bin/bash

# CGI script that takes 3 seconds to complete
echo "Content-Type: text/html"
echo ""
echo "<html><body>"
echo "<h1>Slow CGI Test</h1>"
echo "<p>This script sleeps for 3 seconds...</p>"

# Sleep for 4 seconds
sleep 4

echo "<p>Done! Script completed after 3 seconds.</p>"
echo "<p>Time: $(date)</p>"
echo "</body></html>"
