#include <xbus/socket.h>
#include <xbus/exceptions.h>
#include <xbus/die.h>
#include <unistd.h>

#define BACKLOG 10

xbus::Socket::Socket() {}

xbus::Socket::Socket(const std::string& path) : m_path(path) {
  m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  memset(&m_addr, 0, sizeof(sockaddr_un));
  m_addr.sun_family = AF_UNIX;
  strncpy(m_addr.sun_path, path.c_str(), sizeof(m_addr.sun_path) - 1);
}

xbus::Socket::~Socket() {
  if (m_fd != -1) {
    close();
    m_fd = -1;
  }
}

std::string xbus::Socket::path() {
  return m_path;
}

int xbus::Socket::fd() {
  return m_fd;
}

void xbus::Socket::bind() {
  if (m_fd == -1) return;
  if (::bind(m_fd, (sockaddr*) &m_addr, sizeof(sockaddr_un))) {
    throw SocketException("bind failed");
  }
}

void xbus::Socket::connect() {
  if (m_fd == -1) return;
  if (::connect(m_fd, (sockaddr*) &m_addr, sizeof(sockaddr_un)) == -1) {
    throw SocketException("connect failed");
  }
}

void xbus::Socket::listen() {
  if (::listen(m_fd, BACKLOG) == -1) {
    throw SocketException("listen failed");
  }
}

void xbus::Socket::close() {
  if (m_fd == -1) return;
  ::close(m_fd);
  m_fd = -1;
}

xbus::Socket* xbus::Socket::accept() {
  if (m_fd == -1) return nullptr;
  Socket* socket = new Socket;
  socket->m_fd = ::accept(m_fd, NULL, NULL);
  return socket;
}

void xbus::Socket::write(const std::string& data) {
  if (m_fd == -1) return;
  if(::write(m_fd, data.c_str(), data.size()) == -1) {
    throw IOException("write failed");
  }
}

std::string xbus::Socket::read(size_t size) {
  if (m_fd == -1) return "";
  char* buffer = new char[size+1];
  size_t readSize = ::read(m_fd, buffer, size);
  if (readSize == -1) {
    throw IOException("read failed");
  }
  buffer[size] = 0;
  std::string data(buffer, readSize);
  delete [] buffer;
  return data;
}
