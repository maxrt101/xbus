#ifndef _XBUS_SOCKET_H_
#define _XBUS_SOCKET_H_ 1

#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>

#define XBUS_READ_SIZE 1024

namespace xbus {

constexpr char SOCKET_PATH[] = "/tmp/xbus.sock";

class Socket {
 private:
  std::string m_path;
  sockaddr_un m_addr;
  int m_fd = -1;

 public:
  Socket();
  Socket(const std::string& path);
  ~Socket();

  std::string path();
  int fd();

  void bind();
  void connect();
  void listen();
  void close();

  Socket* accept();

  void write(const std::string& data);
  std::string read(size_t size);
};

} /* namespace xbus */

#endif /* _XBUS_SOCKET_H_ */