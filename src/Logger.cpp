#include "Logger.h"
#include <Arduino.h> // Needed for millis()
#include <time.h>    // Needed for NTP time

Logger logger;

// Constructor
Logger::Logger(size_t maxSize)
    : m_maxSize(maxSize) {}

// Add a log entry with a timestamp
std::string Logger::log(const std::string &message)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Create the log entry with a timestamp
    std::string logEntry = getTimestamp() + " - " + message;

    // Add the new log entry
    m_logs.push_back(logEntry);
    Serial.println(logEntry.c_str());

    // Remove the oldest entry if we exceed the maximum size
    if (m_logs.size() > m_maxSize)
    {
        m_logs.pop_front();
    }
    return logEntry; // Return the log message with timestamp
}

// Retrieve all log entries
std::deque<std::string> Logger::getLogs() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logs; // Return a copy to avoid external modifications
}

// Clear all log entries
void Logger::clearLogs()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.clear();
}

// Retrieve the size of the log
size_t Logger::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logs.size();
}

// Helper function to format the timestamp from the RTC (NTP)
std::string Logger::getTimestamp() const
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return "Timestamp not available";
    }

    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(timestamp);
}
