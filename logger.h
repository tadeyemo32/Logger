#ifndef TADEYEMO32_LOGGER_H
#define TADEYEMO32_LOGGER_H

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <stdexcept>
#include <mutex>
#include <sstream>
#include <string>
#include <memory>

namespace Log {

enum class FormatType { TXT, CSV, JSON, XML };
enum class LogType { INFO, DEBUG, WARNING, ERROR, CRITICAL, UNKNOWN };

inline std::string logTypeToString(LogType t) {
    switch (t) {
        case LogType::INFO:     return "Info";
        case LogType::DEBUG:    return "Debug";
        case LogType::WARNING:  return "Warning";
        case LogType::ERROR:    return "Error";
        case LogType::CRITICAL: return "Critical";
        case LogType::UNKNOWN:  return "Unknown";
        default:                return "Undefined";
    }
}

inline std::string current_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&now_time);
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

inline std::string escape_json(const std::string &input) {
    std::ostringstream oss;
    for (char c : input) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b";  break;
            case '\f': oss << "\\f";  break;
            case '\n': oss << "\\n";  break;
            case '\r': oss << "\\r";  break;
            case '\t': oss << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20 || c == 0x7F)
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                else
                    oss << c;
        }
    }
    return oss.str();
}

class Logger {
private:
    std::ofstream file;
    std::mutex mtx;
    FormatType format;
    bool debugMode;
    size_t maxSizeBytes;
    std::string fileName;
    std::string logDirectory;
    bool firstJsonEntry;
    bool dirCreated;
    bool jsonArrayStarted;

    std::filesystem::path getFullPath() const {
        std::string ext;
        switch (format) {
            case FormatType::TXT:  ext = ".txt"; break;
            case FormatType::CSV:  ext = ".csv"; break;
            case FormatType::JSON: ext = ".json"; break;
            case FormatType::XML:  ext = ".xml"; break;
        }
        
        if (logDirectory.empty()) {
            return std::filesystem::path(fileName + ext);
        }
        
        return std::filesystem::path(logDirectory) / (fileName + ext);
    }

    void rotate_if_needed() {
        if (!file.is_open()) return;
        file.flush();
        std::filesystem::path path = getFullPath();
        if (std::filesystem::exists(path) &&
            std::filesystem::file_size(path) >= maxSizeBytes) {
            file.close();
            std::string timestamp = current_time();
            for (auto &c : timestamp) if (c == ':' || c == ' ') c = '-';
            std::filesystem::rename(path, path.string() + "." + timestamp);
            
            openFile(); 
        }
    }

    std::string serialize(const std::string &msg, LogType type) {
        std::string ts = current_time();
        switch (format) {
            case FormatType::TXT:
                return "[" + ts + "] [" + logTypeToString(type) + "] " + msg;
            case FormatType::CSV:
                return ts + "," + logTypeToString(type) + ",\"" + escape_json(msg) + "\"";
            case FormatType::JSON:
            {
                std::string jsonEntry =
                    "  {\n"
                    "    \"timestamp\": \"" + ts + "\",\n"
                    "    \"log_type\": \"" + logTypeToString(type) + "\",\n"
                    "    \"message\": \"" + escape_json(msg) + "\"\n"
                    "  }";
                return jsonEntry;
            }
            case FormatType::XML:
                return "<log>\n  <timestamp>" + ts + "</timestamp>\n  <type>" +
                       logTypeToString(type) + "</type>\n  <message>" + escape_json(msg) +
                       "</message>\n</log>";
            default:
                return msg;
        }
    }

    void openFile() {
        std::filesystem::path filePath = getFullPath();
        bool fileExists = std::filesystem::exists(filePath);
        
        file.open(filePath, std::ios::out | std::ios::app);
        
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open log file: " + filePath.string());
        }

        if (format == FormatType::JSON) {
            if (!fileExists || std::filesystem::file_size(filePath) == 0) {
                file << "[\n";
                firstJsonEntry = true;
                jsonArrayStarted = true;
            } else {
                file.close();                 
                std::ifstream inFile(filePath, std::ios::in);
                if (inFile.is_open()) {
                    inFile.seekg(-1, std::ios::end);
                    char lastChar;
                    inFile.get(lastChar);
                    inFile.close();
                    
                    file.open(filePath, std::ios::out | std::ios::app);
                    
                    if (lastChar == ']') {
                        file.seekp(-1, std::ios::end);
                        file << ",";
                        firstJsonEntry = false;
                        jsonArrayStarted = true;
                    } else {
                        file.close();
                        file.open(filePath, std::ios::out | std::ios::trunc);
                        file << "[\n";
                        firstJsonEntry = true;
                        jsonArrayStarted = true;
                    }
                }
            }
        }
    }

    Logger(const std::string &fname, FormatType fmt, size_t maxMB, bool dbg, const std::string &dir = "")
        : fileName(fname), format(fmt), debugMode(dbg), maxSizeBytes(maxMB * 1024 * 1024),
          logDirectory(dir), firstJsonEntry(true), dirCreated(false), jsonArrayStarted(false)
    {
        std::filesystem::path dirPath;
        
        if (logDirectory.empty()) {
            dirPath = "../logs";
        } else {
            dirPath = logDirectory;
        }

        if (!std::filesystem::exists(dirPath)) {
            if (std::filesystem::create_directories(dirPath)) {
                dirCreated = true;
                if (debugMode) {
                    std::cout << "[" << current_time() << "] [Debug] Created log directory: "
                              << std::filesystem::absolute(dirPath) << std::endl;
                }
            } else {
                throw std::runtime_error("Failed to create logs directory: " + dirPath.string());
            }
        }

        openFile();

        if (dirCreated) {
            log("Successfully created directory: " + dirPath.string(), LogType::DEBUG);
        }

        log("Logger initialized", LogType::DEBUG);
    }

public:
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    static Logger &getInstance(
        const std::string &fname,
        FormatType fmt = FormatType::TXT,
        size_t maxMB = 10,
        bool dbg = false,
        const std::string &dir = ""
    ) {
        static std::mutex init_mtx;
        std::lock_guard<std::mutex> lock(init_mtx);
        static Logger instance(fname, fmt, maxMB, dbg, dir);
        return instance;
    }

    void log(const std::string &msg, LogType type) {
        std::lock_guard<std::mutex> lock(mtx);
        rotate_if_needed();
        
        if (!file.is_open()) {
            openFile();
        }
        
        if (file.is_open()) {
            if (format == FormatType::JSON) {
                if (!firstJsonEntry) {
                    file << ",\n";
                } else {
                    firstJsonEntry = false;
                }
            }
            
            file << serialize(msg, type);
            
            if (format != FormatType::JSON) {
                file << std::endl;
            }
            
            file.flush();
            
            if (debugMode) {
                std::cout << "[" << current_time() << "] ["
                          << logTypeToString(type) << "] "
                          << msg << std::endl;
            }
        }
    }

    void setDebugStatus(bool status) {
        std::lock_guard<std::mutex> lock(mtx);
        debugMode = status;
    }

    void setLogDirectory(const std::string &dir) {
        std::lock_guard<std::mutex> lock(mtx);
        if (logDirectory != dir) {
            logDirectory = dir;
            if (file.is_open()) {
                if (format == FormatType::JSON && jsonArrayStarted) {
                    file << "\n]";
                }
                file.close();
            }
            
            std::filesystem::path dirPath(dir);
            if (!std::filesystem::exists(dirPath)) {
                std::filesystem::create_directories(dirPath);
            }
            
            openFile();
        }
    }

    ~Logger() {
        std::lock_guard<std::mutex> lock(mtx);
        if (file.is_open()) {
            if (format == FormatType::JSON && jsonArrayStarted) {
                file << "\n]";
            }
            file.close();
        }
    }
};
}

#endif
