#ifndef _XBUS_EXCEPTIONS_H_
#define _XBUS_EXCEPTIONS_H_ 1

#include <exception>
#include <string>

namespace xbus {

class IOException : public std::exception {
  std::string m_msg;

 public:
  IOException(const std::string& msg);
  ~IOException() = default;

  const char* what() const noexcept override;

  void print() const;
};

class SocketException : public std::exception {
  std::string m_msg;

 public:
  SocketException(const std::string& msg);
  ~SocketException() = default;

  const char* what() const noexcept override;

  void print() const;
};

} /* namespace xbus */

#endif /* _XBUS_EXCEPTIONS_H_ */