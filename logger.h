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
    bool firstJsonEntry;
    bool dirCreated = false; 
    std::string dirC;

    std::filesystem::path getFullPath() const {
        std::string ext;
        switch (format) {
            case FormatType::TXT:  ext = ".txt"; break;
            case FormatType::CSV:  ext = ".csv"; break;
            case FormatType::JSON: ext = ".json"; break;
            case FormatType::XML:  ext = ".xml"; break;
        }
        return std::filesystem::path("../logs") / (fileName + ext);
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
            
            file.open(path, std::ios::out | std::ios::app);
            if (format == FormatType::JSON) {
                firstJsonEntry = true;
                file << "[\n";
            }
        }
    }

    std::string serialize(const std::string &msg, LogType type) {
        std::string ts = current_time();
        switch (format) {
            case FormatType::TXT:
                return "[" + ts + "] [" + logTypeToString(type) + "] " + msg;
            case FormatType::CSV:
                return ts + "," + logTypeToString(type) + ",\"" + msg + "\"";
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
                       logTypeToString(type) + "</type>\n  <message>" + msg + 
                       "</message>\n</log>";
        }
        return msg;
    }

    Logger(const std::string &fname, FormatType fmt, size_t maxMB, bool dbg)
        : fileName(fname), format(fmt), debugMode(dbg), maxSizeBytes(maxMB * 1024 * 1024), firstJsonEntry(true)
    {
        std::filesystem::path dirPath = "../logs";
        if (!std::filesystem::exists(dirPath)) {
            if (!std::filesystem::create_directory(dirPath)) {
                throw std::runtime_error("Failed to create logs directory.");
            }
            dirCreated = false ; 
           dirC = current_time(); 
        }

        std::filesystem::path filePath = getFullPath();
        bool fileExists = std::filesystem::exists(filePath);
        
        if (format == FormatType::JSON && fileExists) {
     
            file.open(filePath, std::ios::out | std::ios::in | std::ios::app);
            if (file.is_open()) {
                file.seekp(-1, std::ios::end);
                char lastChar;
                              file << ",";
                firstJsonEntry = false;
            }
        } else {
            file.open(filePath, std::ios::out | std::ios::app);
            if (format == FormatType::JSON) {
                file << "[\n";
            }
        }

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open log file: " + filePath.string());
        }
        if (dirCreated){
           log("Succefully Created Directory",LogType::DEBUG,dirC); 
        }

        log("Logger initialized", LogType::DEBUG);
    }

public:
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    static Logger &getInstance(
        const std::string &fname,
        FormatType fmt = FormatType::TXT,
        bool dbg = false,
        size_t maxMB = 10
    ) {
        static std::mutex init_mtx;
        std::lock_guard<std::mutex> lock(init_mtx);
        static Logger instance(fname, fmt, maxMB, dbg);
        return instance;
    }

    void log(const std::string &msg, LogType type) {
        std::lock_guard<std::mutex> lock(mtx);
        rotate_if_needed();
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




      void log(const std::string &msg, LogType type,std::string t) {
        std::lock_guard<std::mutex> lock(mtx);
        rotate_if_needed();
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
                std::cout << "[" << t << "] ["
                          << logTypeToString(type) << "] "
                          << msg << std::endl;
            }
        }
    }



    void setDebugStatus(bool status) {
        std::lock_guard<std::mutex> lock(mtx);
        debugMode = status;
    }

    ~Logger() {
        std::lock_guard<std::mutex> lock(mtx);
    
        if (file.is_open()) {
            if (format == FormatType::JSON) {
                file << "\n]";
            }
            file.close();
        }
    }
};
}

#endif 
