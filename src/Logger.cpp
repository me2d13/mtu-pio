#include "Logger.h"
#include <Arduino.h> // Needed for millis()
#include <time.h>    // Needed for NTP time
#include <chrono>
#include <iomanip>

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
    m_total++;
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

uint32_t Logger::total() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_total;
}

// Entries logged after the given sequence number, oldest first
std::vector<std::string> Logger::getLogsSince(uint32_t sequence) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> out;

    // sequence number of the oldest entry still present in the ring buffer
    // (entries 1..m_total were logged, only the last m_logs.size() are kept)
    uint32_t oldest = m_total > m_logs.size() ? m_total - static_cast<uint32_t>(m_logs.size()) + 1 : 1;
    uint32_t start = sequence + 1;
    if (start < oldest) start = oldest;

    for (uint32_t i = start; i <= m_total; i++)
    {
        out.push_back(m_logs[i - oldest]);
    }
    return out;
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
    using namespace std::chrono;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        // return milis() if we can't get the time
        return std::to_string(millis());
    }

    // get absolute wall time
    auto now = system_clock::now();

    // find the number of milliseconds
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

    // build output string
    std::ostringstream oss;
    oss.fill('0');

    // convert absolute time to time_t seconds
    // and convert to "date time"
    oss << std::string(timestamp);
    oss << '.' << std::setw(3) << ms.count();

    return oss.str();
}

void Logger::print(const std::string &message)
{
    // add message to buffer
    for (char c : message)
    {
        m_lineBuffer[m_lineBufferIndex++] = c;
    }
}

void Logger::println()
{
    // add newline to buffer
    m_lineBuffer[m_lineBufferIndex++] = '\n';
    // log buffer
    log(std::string(m_lineBuffer, m_lineBufferIndex));
    // clear buffer
    m_lineBufferIndex = 0;
}

void Logger::println(const std::string &message)
{
    print(message);
    println();
}

void Logger::print(const char *message)
{
    print(std::string(message));
}

void Logger::print(const char message)
{
    print(std::string(1, message));
}