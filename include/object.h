#ifndef _XBUS_OBJECT_H_
#define _XBUS_OBJECT_H_ 1

#include <string>
#include <map>

#include <xbus/response.h>
#include <xbus/request.h>
#include <xbus/version.h>
#include <xbus/socket.h>
#include <xbus/log.h>
#include <xbus/die.h>

#include <mrt/threads/pool.h>
#include <mrt/threads/task.h>

namespace xbus {

template <typename T>
class Object {
 public:
  using HandlerType = Response(T::*)(Request);

 private:
  struct HandlingContext {
    Object* object;
    Request request;
  };

 private:
  std::string m_name;
  bool m_running = false;
  Socket* m_socket = nullptr;
  std::map<std::string, std::string> m_fields;
  std::map<std::string, HandlerType> m_properties;

 public:
  inline Object(const std::string& name) : m_name(name) {
    initialize();
  }

  inline ~Object() {
    delete m_socket;
  }

  inline void addField(const std::string& field, const std::string& value = "") {
    m_fields[field] = value;
  }

  inline void addProperty(const std::string& prop, HandlerType handler) {
    m_properties[prop] = handler;
  }

  inline void listen() {
    mrt::ThreadPool<mrt::Task<HandlingContext*>> pool;

    m_running = true;
    while (m_running) {
      std::string data = m_socket->read(XBUS_READ_SIZE);
      if (data.empty()) break;
      // Request request = Request::fromString(data);
      HandlingContext* ctx = new HandlingContext {this, Request::fromString(data)};
      pool.addTask({handleRequestCb, ctx});
    }

    pool.finishAll();
  }

  inline void stop() {
    m_running = false;
  }

  inline bool isRunning() const {
    return m_running;
  }

  virtual inline void onNotify(Request request) {}

 private:
  inline void initialize() {
    m_socket = new Socket(SOCKET_PATH);
    m_socket->connect();
    registerObject();
  }

  inline void checkVersion() const {
    m_socket->write("+version:" + m_name);
    auto response = Response::fromString(m_socket->read(XBUS_READ_SIZE));
    if (response.rest.size() != 1) {
      m_socket->write("+close");
      die("checkVersion: unexpected response");
    }
    if (response.status != "OK") {
      m_socket->write("+close");
      die("check version failed");
    }
    if (response.rest[0] != XBUS_VERSION) {
      m_socket->write("+close");
      die("wrong version: expected: %s, actual: %s", XBUS_VERSION, response.rest[0].c_str());
    }
  }

  inline void registerObject() const {
    m_socket->write("+register:" + m_name);
    std::string result = m_socket->read(XBUS_READ_SIZE);
  }

  inline Response handleRequest(Request request) {
    xbus::info("recv '%s'", request.toString().c_str());

    if (!request.isValid()) {
      xbus::warning("discarding invalid request '%s'", request.toString().c_str());
      return {""};
    }

    if (request.action == xbus::ACTION_PROPERTY) {
      return dispatchMethod(request);
    } else if (request.action == xbus::ACTION_FIELD) {
      if (m_fields.find(request.subject) == m_fields.end()) {
        return {"ERR", {"NO SUCH FIELD"}};
      }
      if (request.request) {
        return {"OK", {m_fields[request.subject]}};
      } else {
        if (request.args.size() != 1) {
          return {"ERR", {"ARGUMENT MISMATCH"}};
        }
        m_fields[request.subject] = request.args[0];
        return {"OK"};
      }
    } else if (request.action == xbus::ACTION_NOTIFY) {
      onNotify(request);
    } else {
      return {"ERR", {"UNKNOWN ACTION"}};
    }

    return {""};
  }

  inline Response dispatchMethod(Request request) {
    if (m_properties.find(request.subject) == m_properties.end()) {
      return {"ERR", {"NO SUCH PROPERTY"}};
    }
    return (((T*)this)->*m_properties[request.subject])(request);
  }

  static inline void handleRequestCb(void* ctx) {
    HandlingContext* context = (HandlingContext*)ctx;
    Response response = context->object->handleRequest(context->request);
    response.tag = context->request.tag;
    if (!response.status.empty()) {
      context->object->m_socket->write(response.toString());
    }
    delete context;
  }
};

} /* namespace xbus */

#endif /* _XBUS_OBJECT_H_ */