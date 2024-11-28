#ifndef LOGGER_H
#define LOGGER_H

#include <deque>
#include <string>
#include <mutex>
#include <sstream>

class Logger {
public:
    explicit Logger(size_t maxSize = 100); // Constructor with a maximum size

    // Add a log entry with a timestamp and return the log message
    std::string log(const std::string& message);

    // Retrieve all log entries
    std::deque<std::string> getLogs() const;

    // Clear all log entries
    void clearLogs();

    // Retrieve the size of the log
    size_t size() const;

private:
    size_t m_maxSize;                // Maximum number of log entries
    mutable std::mutex m_mutex;      // Protect shared data
    std::deque<std::string> m_logs; // Store log entries

    // Helper function to format the timestamp
    std::string getTimestamp() const;
};

extern Logger logger;

#endif // LOGGER_H
