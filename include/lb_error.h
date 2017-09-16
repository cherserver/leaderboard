#ifndef INCLUDE_LB_ERROR_H_
#define INCLUDE_LB_ERROR_H_

#include <exception>
#include <string>

namespace err {
class Error : public std::exception {
public:
	Error(const std::string &type) throw();
	Error(const std::string &type, const std::string &object) throw();
	Error(const std::string &type, const std::string &object, const std::string &value) throw();
	virtual ~Error() throw();
	virtual const char* what() const throw();
private:
	const std::string m_what;
};
} //end of err namespace

#endif /* INCLUDE_LB_ERROR_H_ */
