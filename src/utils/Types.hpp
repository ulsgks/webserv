#ifndef TYPES_HPP
#define TYPES_HPP

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

// Forward declarations
class ServerBlock;
class LocationBlock;
class Connection;
class Socket;
struct PollResult;

// HTTP-related types
typedef std::multimap<std::string, std::string> HeaderMap;
typedef std::map<std::string, std::string> QueryParamMap;
typedef std::map<std::string, std::string> FormDataMap;
typedef std::map<std::string, std::string> CgiEnvironmentMap;
typedef std::vector<std::string> StringVector;
typedef std::vector<std::string> CgiEnvironmentVector;

// Server configuration types
typedef std::pair<std::string, int> ListenPair;  // host:port combination
typedef std::vector<ListenPair> ListenVector;
typedef std::vector<ServerBlock> ServerBlockVector;
typedef std::vector<LocationBlock> LocationBlockVector;
typedef std::map<int, std::string> ErrorPageMap;           // status_code -> error_page_path
typedef std::map<std::string, std::string> CgiHandlerMap;  // extension -> handler_path

// Server runtime types
typedef std::map<int, Socket> SocketMap;                    // port -> Socket
typedef std::map<int, Connection*> ConnectionMap;           // fd -> Connection*
typedef std::map<int, const ServerBlock*> DefaultBlockMap;  // port -> default ServerBlock*
typedef std::map<int, short> EventMap;                      // fd -> events
typedef std::vector<PollResult> PollResultVector;

// CGI-related types
typedef std::pair<std::string, std::string> CgiComponentPair;  // script_path, path_info

// Configuration parsing types
typedef std::vector<std::string> DirectiveValues;
typedef std::pair<std::string, int> ServerNamePortPair;  // server_name, port
typedef std::map<ServerNamePortPair, std::string> ServerNameMap;

// System-level types
typedef std::vector<struct pollfd> PollFdVector;
typedef std::set<ListenPair> ListenPairSet;
typedef std::map<ListenPair, const ServerBlock*> ListenDefaultMap;

// Iterator types for improved readability
typedef HeaderMap::iterator HeaderMapIt;
typedef HeaderMap::const_iterator HeaderMapConstIt;
typedef CgiEnvironmentMap::const_iterator CgiEnvironmentMapConstIt;
typedef QueryParamMap::const_iterator QueryParamMapConstIt;
typedef FormDataMap::const_iterator FormDataMapConstIt;
typedef ErrorPageMap::const_iterator ErrorPageMapConstIt;
typedef CgiHandlerMap::const_iterator CgiHandlerMapConstIt;
typedef StringVector::const_iterator StringVectorConstIt;
typedef std::string::const_iterator StringConstIt;

// Server runtime iterator types
typedef SocketMap::iterator SocketMapIt;
typedef SocketMap::const_iterator SocketMapConstIt;
typedef ConnectionMap::iterator ConnectionMapIt;
typedef ConnectionMap::const_iterator ConnectionMapConstIt;
typedef DefaultBlockMap::const_iterator DefaultBlockMapConstIt;

// Configuration iterator types
typedef ServerBlockVector::iterator ServerBlockVectorIt;
typedef ServerBlockVector::const_iterator ServerBlockVectorConstIt;
typedef LocationBlockVector::iterator LocationBlockVectorIt;
typedef LocationBlockVector::const_iterator LocationBlockVectorConstIt;
typedef ListenVector::const_iterator ListenVectorConstIt;
typedef ServerNameMap::iterator ServerNameMapIt;
typedef ServerNameMap::const_iterator ServerNameMapConstIt;
typedef PollFdVector::iterator PollFdVectorIt;
typedef PollFdVector::const_iterator PollFdVectorConstIt;
typedef ListenPairSet::iterator ListenPairSetIt;
typedef ListenPairSet::const_iterator ListenPairSetConstIt;
typedef ListenDefaultMap::iterator ListenDefaultMapIt;
typedef ListenDefaultMap::const_iterator ListenDefaultMapConstIt;

#endif  // TYPES_HPP
