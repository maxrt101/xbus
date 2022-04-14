#include <xbus/response.h>
#include <xbus/log.h>
#include <mrt/container_utils.h>

xbus::Response::Response(const std::string& status) : status(status) {}

xbus::Response::Response(const std::string& status, std::vector<std::string> rest) : status(status), rest(rest) {}

std::string xbus::Response::toString() const {
  std::string result = mrt::reduce(rest,
    [](auto result, auto element) {
      return result + "," + element;
    }, status);
  if (tag) result += "#" + std::to_string(tag);
  return result;
}

xbus::Response xbus::Response::fromString(const std::string& str) {
  Response response;

  size_t index = 0;
  while (index < str.size() && !mrt::isIn(str[index], ',', '#')) {
    response.status += str[index++];
  }

  if (index < str.size() && str[index] == ',') {
    index++;
    std::string s;
    while (index < str.size() && str[index] != '#') {
      s += str[index++];
      if (str[index] == ',') {
        response.rest.push_back(s);
        s = "";
        index++;
      }
    }
    response.rest.push_back(s);
  }

  if (index < str.size() && str[index] == '#') {
    index++;
    std::string tag;
    while (index < str.size() && isnumber(str[index])) {
      tag += str[index++];
    }
    try {
      response.tag = std::stoi(tag);
    } catch (...) {
      error("Response parsing failed: invalid tag");
    }
  }

  return response;
}
