#ifndef _XBUS_REQUEST_H_
#define _XBUS_REQUEST_H_ 1

#include <string>
#include <vector>

namespace xbus {

constexpr char ACTION_NOTIFY[]   = "!";
constexpr char ACTION_PROPERTY[] = "+";
constexpr char ACTION_METHOD[]   = "+";
constexpr char ACTION_FIELD[]    = "-";

/*
  Format: [object] action subject [args] [async] [request] [tag]
  actions: - + !
  args: : arg , ...
  async: &
  request: ?
  tag: # sender_id
*/
class Request {
 public:
  std::string object;
  std::string action;
  std::string subject;
  std::vector<std::string> args;
  bool request = false;
  bool async = false;
  int tag = 0;

 public:
  Request() = default;
  Request(const Request& rhs) = default;
  ~Request() = default;

  bool isValid() const;
  std::string toString() const;

  static Request fromString(const std::string& str);
};

bool isRequest(const std::string& str);

} /* namespace xbus */

#endif /* _XBUS_REQUEST_H_ */