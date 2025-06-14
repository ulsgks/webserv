#!/usr/bin/env php
<?php
// judging.php - Cookie-based session management demonstration
// This script judges you based on how many times you've visited

// Start output buffering to control headers
ob_start();

// Session configuration
$cookie_name = "webserv_session";
$cookie_lifetime = 3600 * 24; // 24 hours

// Initialize variables
$session_id = "";
$visits = 0;
$reset_requested = false;

// Parse POST data for reset request
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    // In CGI mode, we need to manually parse POST data
    $post_data = file_get_contents('php://stdin');
    parse_str($post_data, $_POST);
    
    if (isset($_POST['reset']) && $_POST['reset'] == '1') {
        // Clear output buffer
        ob_end_clean();
        
        // Send redirect headers
        echo "Status: 303 See Other\r\n";
        echo "Location: /cgi-bin/judging.php\r\n";
        echo "Set-Cookie: " . $cookie_name . "=; Max-Age=0; Path=/; HttpOnly\r\n";
        echo "\r\n";
        
        // Exit to prevent further output
        exit(0);
    }
}

// Parse cookies manually from HTTP_COOKIE environment variable
$cookies = array();
if (isset($_SERVER['HTTP_COOKIE'])) {
    $cookie_string = $_SERVER['HTTP_COOKIE'];
    $cookie_pairs = explode('; ', $cookie_string);
    foreach ($cookie_pairs as $cookie_pair) {
        $parts = explode('=', $cookie_pair, 2);
        if (count($parts) == 2) {
            $name = trim($parts[0]);
            $value = urldecode(trim($parts[1]));
            $cookies[$name] = $value;
        }
    }
}

// Handle session cookie
if (isset($cookies[$cookie_name])) {
    // Parse existing session data
    $session_data = $cookies[$cookie_name];
    $parts = explode(':', $session_data);
    
    if (count($parts) == 2) {
        $session_id = $parts[0];
        $visits = intval($parts[1]);
    }
}

// Generate new session ID if needed
if (empty($session_id)) {
    $session_id = generate_session_id();
    $visits = 0;
}

// Increment visit count
$visits++;

// Prepare cookie value
$cookie_value = $session_id . ':' . $visits;

// Function to generate a simple session ID
function generate_session_id() {
    // Use a combination of time and random data
    $data = time() . ':' . mt_rand(10000, 99999);
    return 'sess_' . substr(md5($data), 0, 16);
}

// Function to get judgment based on visit count
function get_judgment($visits) {
    switch ($visits) {
        case 1:
            return "Welcome, nice to meet you!";
        case 2:
            return "Oh, you're back. Cool.";
        case 3:
            return "Again? I mean... hi, I guess.";
        case 4:
            return "Seriously? Don't you have anything better to do?";
        case 5:
            return "AGAIN? Fine, whatever. I'll be here.";
        case 6:
        case 7:
            return "You know, there are other websites on the internet...";
        case 8:
        case 9:
            return "I'm starting to worry about you. This isn't healthy.";
        case 10:
            return "TEN FUCKING VISITS? Get a life, for fuck's sake!";
        case 11:
            return "I'm not mad, I'm just disappointed. Actually, I'm both.";
        case 12:
            return "You know what? I'm going to start charging rent.";
        case 13:
            return "Unlucky 13. Perfect for your obsessive behavior.";
        case 14:
            return "I've seen stalkers with more restraint than you.";
        case 15:
            return "Fifteen. That's like... three times five. That's too many.";
        case 16:
            return "I'm running out of creative ways to tell you to go away.";
        case 17:
            return "Maybe we should talk about your attachment issues?";
        case 18:
            return "I'm seriously considering a restraining order.";
        case 19:
            return "One more and it's 20. Please don't make it 20.";
        case 20:
            return "TWENTY FUCKING VISITS. I hope you're proud of yourself.";
        default:
            if ($visits > 20 && $visits <= 30) {
                return "I've run out of unique judgments. You've broken me. Are you happy now?";
            } else if ($visits > 30 && $visits <= 50) {
                return "I think we're in this together now. Let's just embrace our codependent relationship.";
            } else if ($visits > 50 && $visits <= 100) {
                return "At this point, I feel like you're just doing this to torture me. Well played.";
            } else if ($visits > 100) {
                return "Okay, you've won. You are the undisputed champion of wasting time on the internet.";
            }
    }
}

// Get the appropriate judgment
$judgment = get_judgment($visits);

// Clear output buffer
ob_end_clean();

// Build and output HTTP headers for CGI
// IMPORTANT: In CGI mode, we must output headers manually
echo "Set-Cookie: " . $cookie_name . "=" . urlencode($cookie_value) . "; ";
echo "Max-Age=" . $cookie_lifetime . "; ";
echo "Path=/; ";
echo "HttpOnly\r\n";
echo "Content-Type: text/html; charset=UTF-8\r\n";
echo "\r\n"; // Empty line to separate headers from body

// Output HTML content
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Browser Judgment Service</title>
    <link rel="stylesheet" href="/fuckingstyle.css">
    <link rel="icon" href="data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>🍪</text></svg>">
    <style>
        form {
            margin-top: 2em;
            margin-bottom: 1.5em;
        }
    </style>
</head>
<body>
    <h1>Your Browser is Fucking Judging You</h1>
    
    <div class="info-box">
        <p><strong>Visit count:</strong> <?php echo $visits; ?></p>
        <p><strong>Session ID:</strong> <?php echo htmlspecialchars($session_id); ?></p>
    </div>
    
    <blockquote>
        <p><?php echo htmlspecialchars($judgment); ?></p>
    </blockquote>
    
    <form method="post" action="/cgi-bin/judging.php">
        <input type="hidden" name="reset" value="1">
        <button type="submit">Stop judging me!</button>
    </form>
    
    <hr>
    
    <h2>How This Fucking Works</h2>
    
    <p>This page demonstrates cookie-based sessions using the server's generic cookie handling:</p>
    
    <ul>
        <li><strong>Session Creation:</strong> When you first visit, a unique session ID is generated</li>
        <li><strong>Cookie Storage:</strong> The session ID and visit count are stored in a cookie</li>
        <li><strong>State Tracking:</strong> Each visit increments the counter in your session</li>
        <li><strong>Persistence:</strong> Your session lasts 24 hours (then the judgment starts over)</li>
        <li><strong>Reset Function:</strong> The button clears your session and starts fresh</li>
    </ul>
    
    <p>The cookie format is simple: <code>session_id:visit_count</code></p>
    
    <p><em>Technical note: This implementation uses the POST-Redirect-GET pattern to prevent 
    browser warnings about form resubmission when refreshing the page.</em></p>
        
    <footer>
        <nav>
            <p><a href="/">← Back to the motherfucking home page</a></p>
        </nav>
    </footer>
</body>
</html>
