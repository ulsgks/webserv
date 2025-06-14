#!/bin/bash

# Output HTTP headers
echo "Content-Type: text/html"
echo ""

# Get basic server info
SERVER_DATE=$(date)
HOSTNAME=$(hostname)
KERNEL=$(uname -sr)
DISK_USAGE=$(df -h / | awk '/\// { print $3 "/" $2 }')

# Generate the HTML
cat << EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Server Status</title>
    <link rel="stylesheet" href="/fuckingstyle.css">
    <link rel="icon" href="data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>💾</text></svg>">
</head>
<body>
    <h1>Motherfucking Server Status</h1>
    
    <pre>
Date       : $SERVER_DATE
Hostname   : $HOSTNAME
Kernel     : $KERNEL
Disk Usage : $DISK_USAGE
    </pre>
    
    <footer>
        <nav>
            <p><a href="/">← Back to the motherfucking home page</a></p>
        </nav>
    </footer>
</body>
</html>