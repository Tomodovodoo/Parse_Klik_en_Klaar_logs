#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <cctype>
#include <algorithm>

namespace fs = std::filesystem;

// Data structure representing a parsed log entry.
struct LogEntry {
    std::string timestamp;  // e.g., "02/19 01:55:01" (normalized) or empty if not provided.
    std::string source;     // e.g., "MMM", "CM", "FOTA", "cellwan", etc.
    std::string log_level;  // e.g., "INFO", "user.notice", "kern.info", etc.
    std::string message;    // The remainder of the log message.
    std::string fileName;   // The original file name (for reference).
};

// Helper: Normalize a timestamp string into the format "mm/dd hh:mm:ss".
// Supports two formats:
// 1. "Mon dd hh:mm:ss" (e.g., "Feb 14 2:20:11") is converted to "02/14 02:20:11".
// 2. "mm/dd h:m:s" is padded as needed.
std::string normalizeTimestamp(const std::string &ts) {
    if (ts.empty()) return ts;
    // Check if the first character is alphabetic (assume month abbreviation)
    if (std::isalpha(ts[0])) {
        std::istringstream iss(ts);
        std::string monthStr;
        int day, hour, minute, second;
        iss >> monthStr >> day;
        std::string timeStr;
        iss >> timeStr; // e.g., "2:20:11"
        char colon;
        std::istringstream timeStream(timeStr);
        timeStream >> hour >> colon >> minute >> colon >> second;
        std::map<std::string, std::string> monthMap = {
            {"Jan","01"}, {"Feb","02"}, {"Mar","03"}, {"Apr","04"},
            {"May","05"}, {"Jun","06"}, {"Jul","07"}, {"Aug","08"},
            {"Sep","09"}, {"Oct","10"}, {"Nov","11"}, {"Dec","12"}
        };
        std::string monthNum = monthMap[monthStr];
        auto twoDigit = [](int num) {
            std::ostringstream oss;
            if (num < 10) oss << "0";
            oss << num;
            return oss.str();
        };
        return monthNum + "/" + twoDigit(day) + " " + twoDigit(hour) + ":" + twoDigit(minute) + ":" + twoDigit(second);
    } else {
        // Assume format is "mm/dd h:m:s" (or already "mm/dd hh:mm:ss")
        size_t spacePos = ts.find(' ');
        if (spacePos != std::string::npos) {
            std::string datePart = ts.substr(0, spacePos); // e.g., "02/19"
            std::string timePart = ts.substr(spacePos + 1);  // e.g., "2:20:11"
            std::istringstream timeStream(timePart);
            int h, m, s;
            char colon;
            timeStream >> h >> colon >> m >> colon >> s;
            auto twoDigit = [](int num) {
                std::ostringstream oss;
                if (num < 10) oss << "0";
                oss << num;
                return oss.str();
            };
            return datePart + " " + twoDigit(h) + ":" + twoDigit(m) + ":" + twoDigit(s);
        }
        return ts;
    }
}

// Updated parseLine: Tries several patterns in order.
// New patterns for CM logs, FOTA logs, and MMM logs have been added.
// MMM logs now correctly separate source and log level.
bool parseLine(const std::string &line, LogEntry &entry) {
    std::smatch match;

    // Pattern for CM logs:
    // Example: [02/19 01:15:01][CM][INFO][MSTC_MI]checkRecovery(338)enableRadio=1,...
    static const std::regex pattern_cm(R"(^\[([\d/]+\s+\d{1,2}:\d{1,2}:\d{1,2})\]\[([^\]]+)\]\[([^\]]+)\](.*)$)");
    if (std::regex_match(line, match, pattern_cm)) {
        entry.timestamp = normalizeTimestamp(match[1].str());
        entry.source = match[2].str();
        entry.log_level = match[3].str();
        entry.message = match[4].str();
        return true;
    }
    
    // Pattern for FOTA logs:
    // Example: [INFO] [FOTA] Get CPE IMEI info Success.
    static const std::regex pattern_fota(R"(^\s*\[([A-Z]+)\]\s*\[([A-Z]+)\]\s+(.*)$)");
    if (std::regex_match(line, match, pattern_fota)) {
        entry.log_level = match[1].str();
        entry.source = match[2].str();
        entry.message = match[3].str();
        // No timestamp in FOTA lines.
        return true;
    }
    
    // Pattern for MMM logs:
    // Example: [MMM][INFO]mtkGetCampSt(4652) DEBUG: rilitem->rfChannel.campOnStatus:1
    // Note: Using \s* so that no space is required after the second bracket.
    static const std::regex pattern_mmm(R"(^\[([^\]]+)\]\[([^\]]+)\]\s*(.*)$)");
    if (std::regex_match(line, match, pattern_mmm)) {
        entry.source = match[1].str();      // Should capture "MMM"
        entry.log_level = match[2].str();     // Should capture "INFO" (or similar)
        entry.message = match[3].str();       // Remainder of the message
        // No timestamp present.
        return true;
    }
    
    // Pattern 1: e.g., "[cellwan] Feb 14 02:20:11 user.notice DALCMD: Attached to schema shared memory"
    static const std::regex pattern1(R"(^\[([^\]]+)\]\s+([A-Z][a-z]{2}\s+\d+\s+\d{1,2}:\d{1,2}:\d{1,2})\s+(\S+)\s+(\S+):\s*(.*)$)");
    if (std::regex_match(line, match, pattern1)) {
        entry.source    = match[1].str();
        entry.timestamp = normalizeTimestamp(match[2].str());
        entry.log_level = match[3].str();
        entry.message   = match[4].str();
        if (!match[5].str().empty())
            entry.message += " " + match[5].str();
        return true;
    }
    
    // Pattern 2: e.g., "Feb 19 01:55:01 user.info zcmdModuleCfg: Enter function zcmdReqObjGet Oid 154176"
    static const std::regex pattern2(R"(^([A-Z][a-z]{2}\s+\d+\s+\d{1,2}:\d{1,2}:\d{1,2})\s+(\S+)\s+(\S+):\s*(.*)$)");
    if (std::regex_match(line, match, pattern2)) {
        entry.timestamp = normalizeTimestamp(match[1].str());
        entry.log_level = match[2].str();
        entry.source    = match[3].str();
        entry.message   = match[4].str();
        return true;
    }
    
    // Pattern 3: e.g., "[12/31 18:35:49]"
    static const std::regex pattern3(R"(^\[([\d/]+\s+\d{1,2}:\d{1,2}:\d{1,2})\]$)");
    if (std::regex_match(line, match, pattern3)) {
        entry.timestamp = normalizeTimestamp(match[1].str());
        return true;
    }
    
    // Fallback: store the entire line as the message.
    entry.message = line;
    return true;
}

// Helper: Escape a field for CSV.
std::string escape_csv(const std::string &field) {
    std::string result = field;
    size_t pos = 0;
    while ((pos = result.find('"', pos)) != std::string::npos) {
        result.insert(pos, "\"");
        pos += 2;
    }
    if (result.find(',') != std::string::npos || result.find('\n') != std::string::npos ||
        result.find('"') != std::string::npos) {
        result = "\"" + result + "\"";
    }
    return result;
}

// Helper: Determine log type from filename.
// For example, "syslog.log" and "syslog.log.1" both yield "syslog".
std::string extractLogType(const std::string &fileName) {
    std::string base = fileName;
    size_t pos = base.find(".log");
    if (pos != std::string::npos) {
        base = base.substr(0, pos);
    }
    while (!base.empty() && std::isdigit(base.back()))
        base.pop_back();
    std::transform(base.begin(), base.end(), base.begin(), ::tolower);
    return base;
}

int main(int argc, char* argv[]) {
    // If no folder argument is provided, assume a folder named "syslog" exists in the same directory.
    fs::path folderPath;
    if (argc < 2) {
        folderPath = fs::current_path() / "syslog";
        std::cout << "No folder argument provided. Defaulting to: " << folderPath << "\n";
    } else {
        folderPath = fs::path(argv[1]);
    }
    
    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        std::cerr << "Error: " << folderPath << " is not a valid folder.\n";
        return 1;
    }
    
    // Map: log type -> vector of log entries.
    std::map<std::string, std::vector<LogEntry>> logMap;
    
    for (const auto& fileEntry : fs::directory_iterator(folderPath)) {
        if (!fs::is_regular_file(fileEntry)) continue;
        
        fs::path filePath = fileEntry.path();
        // Only process files with ".log" or ".txt" extensions.
        if (filePath.extension() != ".log" && filePath.extension() != ".txt")
            continue;
        
        std::ifstream infile(filePath);
        if (!infile) {
            std::cerr << "Could not open file: " << filePath << "\n";
            continue;
        }
        
        std::string line;
        while (std::getline(infile, line)) {
            if (line.empty()) continue;
            LogEntry logEntry;
            if (parseLine(line, logEntry)) {
                logEntry.fileName = filePath.filename().string();
                std::string logType = extractLogType(filePath.filename().string());
                logMap[logType].push_back(logEntry);
            }
        }
    }
    
    // Create an output folder.
    fs::create_directory("output");
    
    // Write one CSV file per log type.
    for (const auto &pair : logMap) {
        std::string logType = pair.first;
        std::string outFileName = "output/" + logType + ".csv";
        std::ofstream outfile(outFileName);
        if (!outfile) {
            std::cerr << "Could not create output file: " << outFileName << "\n";
            continue;
        }
        outfile << "Timestamp,Source,Log Level,Message,File\n";
        for (const auto &entry : pair.second) {
            outfile << escape_csv(entry.timestamp) << ","
                    << escape_csv(entry.source) << ","
                    << escape_csv(entry.log_level) << ","
                    << escape_csv(entry.message) << ","
                    << escape_csv(entry.fileName) << "\n";
        }
        outfile.close();
        std::cout << "Wrote " << pair.second.size() << " entries to " << outFileName << "\n";
    }
    
    std::cout << "Processing complete.\n";
    return 0;
}
