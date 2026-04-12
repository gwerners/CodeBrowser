#pragma once
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

void sigchld_handler(int signum);
void replace_invalid_utf8(std::string& str);
void replace_invalid_utf8(char* str, size_t len);
std::string readFile(const std::string& filename);
bool fileExists(const std::string& filename);
std::vector<std::string> split(const std::string& str, char delimiter);
std::string getCurrentTimeFormatted();
bool saveTimestampToJson(const std::string& filePath);
std::string loadTimestampFromJson(const std::string& filePath);
bool endsWith(const std::string& str, const std::string& suffix);
