#include <lb_error.h>

using namespace std;

namespace err {
Error::Error(const string &type) throw()
: m_what("Type: '" + type + "'") {}

Error::Error(const string &type, const string &object) throw()
: m_what("Type: '" + type + "' Object: '" + object + "'") {}

Error::Error(const string &type, const string &object, const string &value) throw()
: m_what("Type: '" + type + "' Object: '" + object + "' Value: '" + value + "'") {}

Error::~Error() throw() {}

const char* Error::what() const throw() {
	return m_what.c_str();
}

} //end of err namespace
