#!/bin/bash

# This CGI script reports the content type received

echo "Content-Type: text/html"
echo ""

cat << EOF
<!DOCTYPE html>
<html>
<head>
    <title>Content Type Test</title>
    <link rel="stylesheet" href="/fuckingstyle.css">
</head>
<body>
    <h1>Content Type Testing</h1>
    <p>This CGI script shows what content type was received.</p>
    
    <div class="info-box">
        <p><strong>Request Method:</strong> $REQUEST_METHOD</p>
        <p><strong>Content Type:</strong> ${CONTENT_TYPE:-"No Content-Type header"}</p>
        <p><strong>Content Length:</strong> ${CONTENT_LENGTH:-"No Content-Length header"}</p>
    </div>
    
    <h2>Request Body:</h2>
    <pre>
EOF

# Read and display the request body (safely, limiting to first 1000 chars)
if [ "$REQUEST_METHOD" = "POST" ] && [ -n "$CONTENT_LENGTH" ]; then
    head -c 1000
else
    echo "(No body data)"
fi

cat << EOF
    </pre>
    
    <hr>
    <p><a href="/errors">← Back to error testing</a></p>
</body>
</html>
EOF