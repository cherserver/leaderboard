#include <cstdlib>

#include <lb_functions.h>
#include <lb_error.h>

using namespace std;

namespace str {
string GetWord(string &src, char delimiter) {
	string res;
	string::size_type pos = src.find(delimiter);
	if (pos==string::npos) {
		res = src;
		src.clear();
	} else {
		res = src.substr(0, pos);
		src.erase(0, pos + 1);
	}
	return res;
}

int64_t Int64(const string &val) {
	return strtoq(val.c_str(), 0, 10);
}

string Str(int64_t val) {
	char buf[32];
	snprintf(buf, sizeof(buf), "%lld", (long long)val);
	return buf;
}

double Double(const string &val) {
	return atof(val.c_str());
}

string Str(double val, int precision) {
	char buf[32];
	snprintf(buf, sizeof(buf), "%.*f", precision, val);
	return buf;
}
} //end of str namespace

namespace test {
inline bool isdigit(const unsigned char ch) {
	return (ch >= '0' && ch <= '9');
}

inline bool islatin(const unsigned char ch) {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

bool Numeric(const string &data) {
	if (data.empty())
		return false;

	for (auto &d : data) {
		if (!isdigit((const unsigned char) d))
			return false;
	}
	return true;
}

bool Username(const string &data) {
	if (data.empty())
		return false;

	for (auto &d : data) {
		if (!isdigit((const unsigned char) d) && !islatin((const unsigned char) d))
			return false;
	}

	return true;
}
} //end of test namespace

namespace date {
string Format(const SystemTimePoint &time, const string &format) {
	time_t cnv = chrono::system_clock::to_time_t(time);
	struct tm t;
	localtime_r(&cnv, &t);

	char buf[1024];
	auto size = strftime(buf, sizeof(buf), format.c_str(), &t);
	return string(buf, size);
}

SystemTimePoint FromString(const string &date) {
	struct tm tm;

	if (strptime(date.c_str(), "%Y-%m-%d %H:%M:%S", &tm) == nullptr)
		throw err::Error("syscall", "mktime");

	time_t dated = mktime(&tm);
	if (dated == -1)
		throw err::Error("syscall", "mktime");

	return chrono::system_clock::from_time_t(dated);
}

time_t WeekBeginDiff(const struct tm &dateinfo) {
	//Diff all secs, mins, hours, days of week (remember Sunday as week begin)
	return dateinfo.tm_sec +
			dateinfo.tm_min * 60 +
			dateinfo.tm_hour * 60 * 60 +
			(dateinfo.tm_wday - 1) * 24 * 60 * 60;
}

SystemTimePoint GetWeekBegin() {
	auto curtime = chrono::system_clock::now();
	auto curtime_t = chrono::system_clock::to_time_t(curtime);

	struct tm dateinfo;
	if (localtime_r(&curtime_t, &dateinfo) == nullptr)
		throw err::Error("syscall", "localtime_r");

	curtime_t -= WeekBeginDiff(dateinfo);
	return chrono::system_clock::from_time_t(curtime_t);
}

SystemTimePoint GetWeekEnd() {
	auto curtime = chrono::system_clock::now();
	auto curtime_t = chrono::system_clock::to_time_t(curtime);

	struct tm dateinfo;
	if (localtime_r(&curtime_t, &dateinfo) == nullptr)
		throw err::Error("syscall", "localtime_r");

	time_t weeklong = 7 * 24 * 60 * 60;
	curtime_t += (weeklong - WeekBeginDiff(dateinfo)) - 1;
	return chrono::system_clock::from_time_t(curtime_t);
}

SystemTimePoint MinuteLater() {
	auto curtime = chrono::system_clock::now();
	curtime += chrono::minutes(1);
	return curtime;
}
} //end of date namespace
