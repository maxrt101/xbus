#include <xbus/xbus.h>

#include <thread>
#include <chrono>
#include <string>
#include <cstdio>

class Test : public xbus::Object<Test> {
  std::string m_status = "0";

 public:
  Test(const std::string& s) : Object(s) {
    addField("value", "0");
    addProperty("status", &Test::status);
    addProperty("wait", &Test::wait);
    addProperty("stop", &Test::pstop);
  }

  void onNotify(xbus::Request request) override {}

  xbus::Response status(xbus::Request request) {
    if (request.request) {
      return {"OK", {m_status}};
    } else {
      if (request.args.size() != 1) {
        return {"ERR", {"ARGUMENT MISMATCH"}};
      }
      m_status = request.args[0];
      return {"OK"};
    }
  }

  xbus::Response wait(xbus::Request request) {
    if (request.request) {
      return {"OK"};
    } else {
      if (request.args.size() != 1) {
        return {"ERR", {"ARGUMENT MISMATCH"}};
      }
      try {
        std::this_thread::sleep_for(std::chrono::seconds(std::stoi(request.args[0])));
      } catch (...) {
        return {"ERR", {"INVALID ARGUMENT"}};
      }
      return {"OK", {"WAIT", request.args[0]}};
    }
  }

  xbus::Response pstop(xbus::Request request) {
    if (request.request) {
      return {"OK", { isRunning() ? "1" : "0" }};
    } else {
      stop();
      return {"OK"};
    }
  }

};

int main(int argc, char ** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s OBJECT\n", argv[0]);
    return 1;
  }

  Test test(argv[1]);
  test.listen();

  return 0;
}