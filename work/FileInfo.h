#pragma once
#include <string>
#include <strings.h>
#include <chrono>

class FileInfo {
public:
    std::string path;
    std::string name;
    std::string access_date;
    uintmax_t size;
    std::string type;
    bool isDir;
    bool operator < (const FileInfo& other) const
    {
        if (isDir && !other.isDir) {
            return true;
        }
        if (!isDir && other.isDir) {
            return false;
        }
        return strcasecmp(name.c_str(),other.name.c_str()) < 0;
    }
};

template <typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
                                                        + system_clock::now());
    return system_clock::to_time_t(sctp);
}
