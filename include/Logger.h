#ifndef LOGGER_H
#define LOGGER_H

#include <deque>
#include <string>
#include <mutex>
#include <sstream>
#include <vector>

class Logger {
public:
    explicit Logger(size_t maxSize = 100); // Constructor with a maximum size

    // Add a log entry with a timestamp and return the log message
    std::string log(const std::string& message);

    // Retrieve all log entries
    std::deque<std::string> getLogs() const;

    // Retrieve entries logged after a given sequence number (monotonic counter)
    std::vector<std::string> getLogsSince(uint32_t sequence) const;

    // Number of entries ever logged, used by clients as a cursor
    uint32_t total() const;

    // Clear all log entries
    void clearLogs();

    // Retrieve the size of the log
    size_t size() const;
    void print(const std::string& message);
    void println(const std::string& message);
    void print(const char* message);
    void print(const char message);
    void print(int value) { print(std::to_string(value)); }
    void print(float value) { print(std::to_string(value)); }
    void println(int value) { println(std::to_string(value)); }
    void println();

private:
    size_t m_maxSize;                // Maximum number of log entries
    mutable std::mutex m_mutex;      // Protect shared data
    std::deque<std::string> m_logs; // Store log entries
    uint32_t m_total = 0;            // How many entries were ever logged (monotonic)
    char m_lineBuffer[500];          // Buffer for formatting log entries
    int m_lineBufferIndex = 0;       // Index in the buffer

    // Helper function to format the timestamp
    std::string getTimestamp() const;
};

extern Logger logger;

#endif // LOGGER_H
