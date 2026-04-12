#pragma once
#include <string>
#include "lsp/PipeAdapter.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// Unified LSP client replacing 10+ duplicate message classes.
// All LSP requests follow the same pattern: build JSON-RPC, send, receive, parse.
class LspClient {
 public:
  LspClient(const std::string& lspCmd);

  // Lifecycle
  void didOpen(const std::string& filePath, const std::string& content);
  void didClose(const std::string& filePath);

  // Queries - all return parsed JSON responses
  json hover(const std::string& filePath, int line, int column);
  json definition(const std::string& filePath, int line, int column);
  json declaration(const std::string& filePath, int line, int column);
  json references(const std::string& filePath, int line, int column);
  json typeDefinition(const std::string& filePath, int line, int column);
  json implementation(const std::string& filePath, int line, int column);
  json typeHierarchy(const std::string& filePath, int line, int column);
  json switchSourceHeader(const std::string& filePath);

 private:
  // Send a position-based request (definition, declaration, etc.)
  json sendPositionRequest(const std::string& method,
                           const std::string& filePath,
                           int line,
                           int column);

  // Send request and receive parsed response
  json sendAndReceive(const json& request);

  // Extract URI from various response formats
  static std::string extractUri(const json& j);

  PipeAdapter* _adapter;
  std::string _lspCmd;
};

// Response parsers
namespace LspResponse {

// Parse hover response into {description: "..."} or {}
json parseHover(const json& response);

// Parse location response (definition/declaration/etc.) - extract first URI
json parseLocation(const json& response);

// Parse references response into array of {uri, line}
json parseReferences(const json& response);

// Parse type hierarchy response
json parseTypeHierarchy(const json& response);

// Parse switch source/header response
json parseSwitchHeader(const json& response);

}  // namespace LspResponse
