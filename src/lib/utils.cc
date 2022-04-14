#include <xbus/utils.h>

std::vector<std::string> xbus::splitString(const std::string& str, char delimiter) {
  std::vector<std::string> result;

  size_t last = 0, current = 0;
  while (current < str.size()) {
    if (str[current] == delimiter) {
      result.push_back(str.substr(last, current-last));
      last = current+1;
    }
    current++;
  }
  if (!str.substr(last, current-last).empty()) {
    result.push_back(str.substr(last, current-last));
  }
  return result;
}
