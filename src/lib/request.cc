#include <xbus/request.h>
#include <xbus/log.h>
#include <mrt/format.h>
#include <mrt/container_utils.h>
#include <cctype>
#include <cstdio>

bool xbus::isRequest(const std::string& str) {
  return Request::fromString(str).isValid();
}

bool xbus::Request::isValid() const {
  return action != "" && subject != "";
}

std::string xbus::Request::toString() const {
  std::string argsstr = mrt::reduce(args,
    [](auto result, auto element) {
      return result + element + ",";
    }, std::string());
  argsstr.pop_back();

  return mrt::format("{}{}{}{}{}{}{}",
    object,
    action,
    subject,
    args.size() ? (action == ACTION_FIELD ? "=" : ":") + argsstr : "",
    async ? "&" : "",
    request ? "?" : "",
    tag ? "#" + std::to_string(tag) : ""
  );
}

xbus::Request xbus::Request::fromString(const std::string& str) {
  Request request;

  size_t index = 0;
  while (index < str.size() && !mrt::isIn(str[index], '+', '-', '!')) {
    request.object += str[index++];
  }

  while (index < str.size() && !isalnum(str[index]) && str[index] != '_') {
    request.action += str[index++];
  }

  while (index < str.size() && (isalnum(str[index]) || str[index] == '_')) {
    request.subject += str[index++];
  }

  if (index < str.size() && mrt::isIn(str[index], ':', '=')) {
    index++;
    std::string arg;
    while (index < str.size() && !mrt::isIn(str[index], '?', '&', '#')) {
      arg += str[index++];
      if (str[index] == ',') {
        request.args.push_back(arg);
        arg = "";
        index++;
      }
    }
    request.args.push_back(arg);
  }

  if (index < str.size() && str[index] == '&') {
    request.async = true;
    index++;
  }

  if (index < str.size() && str[index] == '?') {
    request.request = true;
    index++;
  }

  if (index < str.size() && str[index] == '#') {
    index++;
    std::string tag;
    while (index < str.size() && isnumber(str[index])) {
      tag += str[index++];
    }
    try {
      request.tag = std::stoi(tag);
    } catch (...) {
      error("Request parsing failed: invalid tag");
    }
  }

  return request;
}
