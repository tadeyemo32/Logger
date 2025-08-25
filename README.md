
# Logger - Header-Only C++ Logging Library

**Version:** 2.0  
**Author:** Tadeyemo32  
**License:** MIT  

---

## Overview

`Logger` is a **thread-safe, header-only C++ logging library** designed for professional applications. It supports multiple log formats, runtime debug control, and automatic log rotation. The library is easy to integrate and requires no external dependencies beyond the C++17 standard library.

**Features:**

- Header-only, simple to include.
- Thread-safe for concurrent logging in multithreaded applications.
- Supports multiple log formats: **TXT, CSV, JSON, XML**.
- Configurable debug mode for console output.
- Automatic log rotation based on file size.
- Singleton pattern ensures a single logger instance per application.
- Easy runtime toggle for debug/console logging.
- Logs include **timestamps**, **log levels**, and **serialized data** for future parsing.

---

## Installation

Simply include the header in your project:

```cpp
#include "logger.h"
```

Ensure the library is in your include path or project directory. No compilation or linking is required.

---

## Usage

### Basic Example

```cpp
#include "logger.h"

int main() {
    // Create logger instance: filename, format, max size in MB (optional), debug mode (optional)
    auto &logger = Log::Logger::getInstance("server_logs", Log::FormatType::JSON, 5, true);

    logger.log("Server started", Log::LogType::INFO);
    logger.log("New connection received", Log::LogType::DEBUG);

    // Change debug status at runtime
    logger.setDebugStatus(false);
    logger.log("Debug output disabled", Log::LogType::INFO);

    return 0;
}
```

### Multiple Formats

```cpp
auto &txtLogger = Log::Logger::getInstance("app_txt", Log::FormatType::TXT, 10, true);
auto &csvLogger = Log::Logger::getInstance("app_csv", Log::FormatType::CSV, 10);
auto &xmlLogger = Log::Logger::getInstance("app_xml", Log::FormatType::XML, 5, true);
```

- **TXT**: `[timestamp] [level] message`  
- **CSV**: `timestamp,level,message`  
- **JSON**: `{ "timestamp": "...", "log_type": "...", "message": "..." }`  
- **XML**: `<log><timestamp>...</timestamp><type>...</type><message>...</message></log>`

---

## API Reference

### Logger

#### `static Logger& getInstance(const std::string &filename, FormatType format, size_t maxSizeMB = 10, bool debug = false)`

Get the singleton instance of `Logger`.  
- **filename**: Name of the log file (without extension)  
- **format**: `FormatType` (TXT, CSV, JSON, XML)  
- **maxSizeMB**: Maximum log file size in MB before rotation (optional)  
- **debug**: Enable/disable console output (optional)  

#### `void log(const std::string &message, LogType type)`

Logs a message with the specified severity. Automatically includes timestamp.  

#### `void setDebugStatus(bool status)`

Toggle debug output to the console at runtime.

---

### LogType Enum

- `INFO`  
- `DEBUG`  
- `WARNING`  
- `ERROR`  
- `CRITICAL`  
- `UNKNOWN`  

---

### FormatType Enum

- `TXT`  
- `CSV`  
- `JSON`  
- `XML`  

---

## Example Output

**JSON Log Example:**

```json
{
  "timestamp": "2025-08-26 05:21:00",
  "log_type": "Info",
  "message": "Logger initialized"
}
```

**TXT Log Example:**

```
[2025-08-26 05:21:00] [Info] Logger initialized
```

**CSV Log Example:**

```
2025-08-26 05:21:00,Info,Logger initialized
```

**XML Log Example:**

```xml
<log>
  <timestamp>2025-08-26 05:21:00</timestamp>
  <type>Info</type>
  <message>Logger initialized</message>
</log>
```

---

## Notes

- Singleton ensures only one instance exists across all files.  
- Thread-safe with internal mutex locking.  
- File rotation automatically renames old logs when exceeding `maxSizeMB`.  
- Console debug mode is optional and can be toggled at runtime.  

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
