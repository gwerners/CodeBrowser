#include "lsp/LspClient.h"
#include "fmt/color.h"
#include "fmt/format.h"

static bool s_initialized = false;

LspClient::LspClient(const std::string& lspCmd)
    : _adapter(PipeAdapter::getInstance(lspCmd)), _lspCmd(lspCmd) {
  if (!s_initialized) {
    json init = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "initialize"},
        {"params",
         {{"processId", getpid()},
          {"rootUri", ""},
          {"capabilities", {{"workspace", {{"applyEdit", false}}}}}}}};
    sendAndReceive(init);
    s_initialized = true;
  }
}

void LspClient::didOpen(const std::string& filePath,
                        const std::string& content) {
  json msg = {{"jsonrpc", "2.0"},
              {"method", "textDocument/didOpen"},
              {"params",
               {{"textDocument",
                 {{"uri", "file://" + filePath},
                  {"languageId", "cpp"},
                  {"version", 1},
                  {"text", content}}}}}};
  _adapter->sendNotification(msg.dump());
}

void LspClient::didClose(const std::string& filePath) {
  json msg = {
      {"jsonrpc", "2.0"},
      {"method", "textDocument/didClose"},
      {"params",
       {{"textDocument", {{"uri", "file://" + filePath}, {"version", 1}}}}}};
  _adapter->sendNotification(msg.dump());
}

json LspClient::sendPositionRequest(const std::string& method,
                                    const std::string& filePath,
                                    int line,
                                    int column) {
  json msg = {
      {"jsonrpc", "2.0"},
      {"id", 1},
      {"method", method},
      {"params",
       {{"textDocument", {{"uri", "file://" + filePath}}},
        {"position", {{"line", line - 1}, {"character", column - 1}}}}}};
  return sendAndReceive(msg);
}

json LspClient::sendAndReceive(const json& request) {
  fmt::print(fg(fmt::color::blue), "LSP request: {}\n",
             request.value("method", "?"));
  std::string response = _adapter->sendReceive(request.dump());
  if (response.empty())
    return json{};
  try {
    return json::parse(response);
  } catch (...) {
    fmt::print(fg(fmt::color::red), "Failed to parse LSP response\n");
    return json{};
  }
}

json LspClient::hover(const std::string& filePath, int line, int column) {
  json resp = sendPositionRequest("textDocument/hover", filePath, line, column);
  return LspResponse::parseHover(resp);
}

json LspClient::definition(const std::string& filePath,
                           int line,
                           int column) {
  json resp =
      sendPositionRequest("textDocument/definition", filePath, line, column);
  return LspResponse::parseLocation(resp);
}

json LspClient::declaration(const std::string& filePath,
                            int line,
                            int column) {
  json resp =
      sendPositionRequest("textDocument/declaration", filePath, line, column);
  return LspResponse::parseLocation(resp);
}

json LspClient::references(const std::string& filePath,
                           int line,
                           int column) {
  json resp =
      sendPositionRequest("textDocument/references", filePath, line, column);
  return resp;  // Raw response, parsed by server route
}

json LspClient::typeDefinition(const std::string& filePath,
                               int line,
                               int column) {
  json resp = sendPositionRequest("textDocument/typeDefinition", filePath, line,
                                  column);
  return LspResponse::parseLocation(resp);
}

json LspClient::implementation(const std::string& filePath,
                               int line,
                               int column) {
  json resp = sendPositionRequest("textDocument/implementation", filePath, line,
                                  column);
  return LspResponse::parseLocation(resp);
}

json LspClient::typeHierarchy(const std::string& filePath,
                              int line,
                              int column) {
  json msg = {
      {"jsonrpc", "2.0"},
      {"id", 1},
      {"method", "textDocument/typeHierarchy"},
      {"params",
       {{"textDocument", {{"uri", "file://" + filePath}}},
        {"resolve", 999},
        {"direction", 2},
        {"position", {{"line", line - 1}, {"character", column - 1}}}}}};
  json resp = sendAndReceive(msg);
  return LspResponse::parseTypeHierarchy(resp);
}

json LspClient::switchSourceHeader(const std::string& filePath) {
  json msg = {{"jsonrpc", "2.0"},
              {"id", 1},
              {"method", "textDocument/switchSourceHeader"},
              {"params", {{"uri", "file://" + filePath}}}};
  json resp = sendAndReceive(msg);
  return LspResponse::parseSwitchHeader(resp);
}

std::string LspClient::extractUri(const json& j) {
  if (j.contains("uri"))
    return j["uri"];
  if (j.contains("targetUri"))
    return j["targetUri"];
  return "";
}

// --- Response Parsers ---

namespace LspResponse {

json parseHover(const json& response) {
  if (response.empty())
    return json{};

  // Skip diagnostic notifications
  if (response.contains("method")) {
    std::string method = response["method"];
    if (method == "textDocument/publishDiagnostics")
      return json{};
  }

  if (!response.contains("result") || response["result"].is_null())
    return json{};

  auto& result = response["result"];
  if (!result.contains("contents"))
    return json{};

  auto& contents = result["contents"];
  if (contents.is_null() || !contents.contains("value"))
    return json{};

  std::string value = contents["value"];
  if (value.substr(0, 7) == "No Info")
    return json{};

  return {{"id", 1}, {"jsonrpc", "2.0"}, {"description", value}};
}

json parseLocation(const json& response) {
  if (response.empty())
    return json{};
  if (!response.contains("result"))
    return json{};

  auto& result = response["result"];
  if (result.is_null())
    return json{};

  // Result can be a single location or an array
  json firstResult;
  if (result.is_array() && !result.empty())
    firstResult = result[0];
  else if (result.is_object())
    firstResult = result;
  else
    return json{};

  std::string uri;
  if (firstResult.contains("uri"))
    uri = firstResult["uri"];
  else if (firstResult.contains("targetUri"))
    uri = firstResult["targetUri"];

  int line = 0;
  if (firstResult.contains("range"))
    line = firstResult["range"]["start"]["line"];
  else if (firstResult.contains("targetRange"))
    line = firstResult["targetRange"]["start"]["line"];

  return {{"uri", uri}, {"line", line}};
}

json parseReferences(const json& response) {
  json result = json::array();
  if (response.empty() || !response.contains("result"))
    return result;

  auto& resultArray = response["result"];
  if (!resultArray.is_array())
    return result;

  for (auto& entry : resultArray) {
    std::string uri;
    int line = 0;
    if (entry.contains("uri"))
      uri = entry["uri"];
    if (entry.contains("range"))
      line = entry["range"]["start"]["line"];
    result.push_back({{"uri", uri}, {"line", line}});
  }
  return result;
}

json parseTypeHierarchy(const json& response) {
  if (response.empty() || !response.contains("result"))
    return json{};
  return response["result"];
}

json parseSwitchHeader(const json& response) {
  if (response.empty() || !response.contains("result"))
    return json{};
  return {{"path", response["result"]}};
}

}  // namespace LspResponse
