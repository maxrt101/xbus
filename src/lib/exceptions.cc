#include <xbus/exceptions.h>
#include <mrt/console/colors.h>
#include <cstdio>

xbus::IOException::IOException(const std::string& msg) : m_msg(msg) {}

const char* xbus::IOException::what() const noexcept {
  return m_msg.c_str();
}

void xbus::IOException::print() const {
  fprintf(stderr, "[%s IOException %s]: %s\n", mrt::console::RED, mrt::console::RESET, m_msg.c_str());
}

xbus::SocketException::SocketException(const std::string& msg) : m_msg(msg) {}

const char* xbus::SocketException::what() const noexcept {
  return m_msg.c_str();
}

void xbus::SocketException::print() const {
  fprintf(stderr, "[%s SocketException %s]: %s\n", mrt::console::RED, mrt::console::RESET, m_msg.c_str());
}
