#include "storeforge/core/DateTime.hpp"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace storeforge::core {

DateTime DateTime::fromIsoString(const std::string& isoStr) {
    std::tm tm = {};
    std::istringstream ss(isoStr);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) {
        // Try fallback format without T
        ss.clear();
        ss.str(isoStr);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (ss.fail()) {
            return fromDateString(isoStr);
        }
    }
    std::time_t t = std::mktime(&tm);
    return DateTime(SystemClock::from_time_t(t));
}

DateTime DateTime::fromDateString(const std::string& dateStr) {
    std::tm tm = {};
    std::istringstream ss(dateStr);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) {
        return DateTime::now();
    }
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    std::time_t t = std::mktime(&tm);
    return DateTime(SystemClock::from_time_t(t));
}

std::string DateTime::toIsoString() const {
    std::time_t t = SystemClock::to_time_t(timePoint_);
    std::tm tm = *std::gmtime(&t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string DateTime::toDateString() const {
    std::time_t t = SystemClock::to_time_t(timePoint_);
    std::tm tm = *std::localtime(&t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d");
    return ss.str();
}

std::string DateTime::toDisplayString() const {
    std::time_t t = SystemClock::to_time_t(timePoint_);
    std::tm tm = *std::localtime(&t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

int DateTime::daysUntil(const DateTime& futureDate) const {
    std::time_t t1 = SystemClock::to_time_t(timePoint_);
    std::time_t t2 = SystemClock::to_time_t(futureDate.timePoint_);
    std::tm tm1 = *std::localtime(&t1);
    std::tm tm2 = *std::localtime(&t2);
    tm1.tm_hour = 0;
    tm1.tm_min = 0;
    tm1.tm_sec = 0;
    tm2.tm_hour = 0;
    tm2.tm_min = 0;
    tm2.tm_sec = 0;
    std::time_t day1 = std::mktime(&tm1);
    std::time_t day2 = std::mktime(&tm2);
    double diffSeconds = std::difftime(day2, day1);
    return static_cast<int>(diffSeconds / 86400.0);
}

bool DateTime::isPast(const DateTime& comparisonPoint) const {
    return timePoint_ < comparisonPoint.timePoint_;
}

DateTime DateTime::addDays(int days) const {
    return DateTime(timePoint_ + std::chrono::hours(days * 24));
}

DateTime DateTime::subtractDays(int days) const {
    return DateTime(timePoint_ - std::chrono::hours(days * 24));
}

} // namespace storeforge::core
