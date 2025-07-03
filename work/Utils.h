#pragma once
#include <map>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

void sigchld_handler(int signum);
void replace_invalid_utf8(std::string& str);
std::string readFile(const std::string& filename);
bool exists(const std::string& filename);
std::vector<std::string> split(const std::string& str, char c);

std::string getCurrentTimeFormatted();
bool saveTimestampToJson(const std::string& filePath);
std::string loadTimestampFromJson(const std::string& filePath);
