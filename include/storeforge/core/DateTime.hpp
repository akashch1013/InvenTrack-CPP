#pragma once

#include <string>
#include <chrono>
#include <ctime>

namespace storeforge::core {

class DateTime {
public:
    using SystemClock = std::chrono::system_clock;
    using TimePoint = SystemClock::time_point;

    DateTime() : timePoint_(SystemClock::now()) {}
    explicit DateTime(TimePoint tp) : timePoint_(tp) {}

    static DateTime now() {
        return DateTime(SystemClock::now());
    }

    static DateTime fromIsoString(const std::string& isoStr);
    static DateTime fromDateString(const std::string& dateStr); // YYYY-MM-DD

    [[nodiscard]] std::string toIsoString() const;
    [[nodiscard]] std::string toDateString() const; // YYYY-MM-DD
    [[nodiscard]] std::string toDisplayString() const; // YYYY-MM-DD HH:MM:SS

    [[nodiscard]] TimePoint getTimePoint() const noexcept { return timePoint_; }

    [[nodiscard]] int daysUntil(const DateTime& futureDate) const;
    [[nodiscard]] bool isPast(const DateTime& comparisonPoint = DateTime::now()) const;

    DateTime addDays(int days) const;
    DateTime subtractDays(int days) const;

    bool operator==(const DateTime& other) const noexcept { return timePoint_ == other.timePoint_; }
    bool operator!=(const DateTime& other) const noexcept { return timePoint_ != other.timePoint_; }
    bool operator<(const DateTime& other) const noexcept { return timePoint_ < other.timePoint_; }
    bool operator<=(const DateTime& other) const noexcept { return timePoint_ <= other.timePoint_; }
    bool operator>(const DateTime& other) const noexcept { return timePoint_ > other.timePoint_; }
    bool operator>=(const DateTime& other) const noexcept { return timePoint_ >= other.timePoint_; }

private:
    TimePoint timePoint_;
};

} // namespace storeforge::core
