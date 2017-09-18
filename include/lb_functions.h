#ifndef INCLUDE_LB_FUNCTIONS_H_
#define INCLUDE_LB_FUNCTIONS_H_

#include <string>
#include <chrono>

namespace str {
std::string GetWord(std::string &src, char delimiter);
int64_t Int64(const std::string &val);
std::string Str(int64_t val);
double Double(const std::string &val);
std::string Str(double val, int precision);
} //end of str namespace

namespace test {
bool Numeric(const std::string &data);
bool Username(const std::string &data);
} //end of test namespace

namespace date {
typedef std::chrono::time_point<std::chrono::system_clock> SystemTimePoint;
typedef std::chrono::time_point<std::chrono::steady_clock> SteadyTimePoint;
typedef std::chrono::duration<int, std::milli> MillisecondsDuration;

std::string Format(const SystemTimePoint &time, const std::string &format = "%F %H:%M:%S");
SystemTimePoint FromString(const std::string &date);
SystemTimePoint GetWeekBegin();
SystemTimePoint GetWeekEnd();
SystemTimePoint MinuteLater();
} //end of date namespace

#endif /* INCLUDE_LB_FUNCTIONS_H_ */
