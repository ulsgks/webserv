<h1 align="center">
	A fucking Webserv
</h1>
<p align="center">
	Did this project help you? Give it a 🌟!
</p>

## 🌐 General information
Subject version: 21.2. Bonus included.<br>
HTTP/1.1-compliant web server written in C++98.<br>
Built following RFC 7230-7235 specifications and nginx behavior.<br>
Made by [Ulysse](https://github.com/ulyssegerkens), [Mehdi](https://github.com/) & [Topaze](https://github.com/)<br>

## 🚀 Set-up

### Clone the project
```bash
# Clone with test submodule
git clone --recurse-submodules https://github.com/ulsgks/webserv.git
```

### Build and run
1. Compile the project with `make`.
2. Adapt interpreter paths in `webserv.conf`
    Example: `cgi_handler .php /usr/bin/php`
3. Start the server with `./webserv webserv.conf`
    You can also enable debug logs with `-v`
4. Access the demo hosted by the server in your browser: `http://localhost:8080/`
5. Explore features through the satirical demo website

## 🧪 Tester
A comprehensive testing framework is included as a Git submodule in the `tests/` directory. 
The complete testing suite is also available separately at: [**github.com/ulsgks/webserv-tester**](https://github.com/ulsgks/webserv-tester)

**To run the tests:**
```bash
./tests/run_test.sh
```

Testing Results: 
- Functional tests: 200+ scenarios covered
- Stress tests: Handles 1000+ concurrent connections
- CGI tests: All interpreters working correctly
- Edge cases: Malformed requests, large payloads, timeouts
