#ifndef _XBUS_RESPONSE_H_
#define _XBUS_RESPONSE_H_ 1

#include <string>
#include <vector>

namespace xbus {

/*
  Format: STATUS [rest] [tag]
  rest: , arg ...
  tag: # sender_id
*/

class Response {
 public:
  std::string status;
  std::vector<std::string> rest;
  int tag = 0; 

 public:
  Response() = default;
  Response(const std::string& status);
  Response(const std::string& status, std::vector<std::string> rest);
  ~Response() = default;

  std::string toString() const;

  static Response fromString(const std::string& str);
};

} /* namespace xbus */

#endif /* _XBUS_RESPONSE_H_ */