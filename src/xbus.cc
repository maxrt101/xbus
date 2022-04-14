#include <xbus/xbus.h>
#include <xbus/utils.h>

#include <iostream>
#include <string>
#include <cstdio>

#define _XBUS_CHECK_ARGV() \
  do { \
    if (i + 1 >= argc) { \
      xbus::error("'%s' requires an option", argv[i]); \
      usage(argv[0]); \
      return 1; \
    } \
  } while (0)

static void printData(xbus::Socket& socket, const std::string& data) {
  if (xbus::isRequest(data)) {
    auto request = xbus::Request::fromString(data);
    if (request.action == xbus::ACTION_NOTIFY) {
      printf("%s\n", request.toString().c_str());
    } else {
      printf("%s\n", request.toString().c_str());
      socket.write("ERR,UNSUPPORTED\0");
    }
  } else {
    printf("%s\n", xbus::Response::fromString(data).toString().c_str());
  }
}

static void repl(const std::string& sock) {
  printf("xbus v%s\n", XBUS_VERSION);

  xbus::Socket socket(sock);
  socket.connect();
  
  while (1) {
    printf("] ");
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) continue;
    if (input == "/q" || input == "/quit" || input == "/exit") {
      socket.write("+close");
      break;
    }
    socket.write(input + '\0');

    std::string buffer = socket.read(XBUS_READ_SIZE);

    if (buffer.empty()) break;

    if (buffer.find('\0') != std::string::npos) {
      auto dataSplit = xbus::splitString(buffer, '\0');
      for (auto element : dataSplit) {
        printData(socket, element);
      }
    } else {
      printData(socket, buffer);
    }
  }
}

void usage(const char* argv0) {
  fprintf(stderr,
    "xbus v%s\n"
    "Usage: %s [OPTIONS] COMMAND [ARGS...]\n"
    "Commands:\n"
    "  help                       - Shows this mwssage\n"
    "  version                    - Show version\n"
    "  list                       - Shows list of registered objects\n"
    "  call OBJECT NAME [ARGS]    - Sends a property call\n"
    "  request OBJECT NAME [ARGS] - Sends a property request\n"
    "  notify OBJECT NAME [ARGS]  - Sends a notification\n"
    "  get OBJECT NAME            - Gets a field\n"
    "  set OBJECT NAME VALUE      - Sets a field\n"
    "  send REQUEST               - Send raw request\n"
    "  wait                       - Wait for an object\n"
    "  listen [OBJECT]            - Listen for notifications\n"
    "  parse_req WHAT REQUEST     - WHAT can be 'object', 'action', 'subject', \n"
    "                               'request', 'async' or number for arg in args\n"
    "  parse_res WHAT RESPONSE    - WHAT can be 'status' or number for arg in args\n"
    "Options:\n"
    "  -s SOCK, --socket SOCK - Unix socket for xbusd\n"
    // "  -t T, --timeout T      - \n"
    "  -l LVL, --loglevel LVL - Sets log level (debug, info, warning, error, fatal)\n"
    "If COMMAND is empty - REPL will run\n"
    "To call/notify global property, put '-' instead of OBJECT\n"
    "", XBUS_VERSION, argv0);
}

std::string sendRequest(const std::string& sock, const std::string& request) {
  xbus::Socket socket(sock);
  socket.connect();
  socket.write(request + '\0');
  return socket.read(XBUS_READ_SIZE);
}

int main(int argc, char** argv) {
  std::string command;
  std::string sock = xbus::SOCKET_PATH;
  int threads = 0;

  int i = 1;
  for (; i < argc; i++) {
    if (!strcmp("-s", argv[i]) || !strcmp("--socket", argv[i])) {
      _XBUS_CHECK_ARGV();
      sock = argv[++i];
    } else if (!strcmp("-l", argv[i]) || !strcmp("--loglevel", argv[i])) {
      xbus::setLogLevel(xbus::stringToLogLevel(argv[++i]));
    } else {
      if (command.empty()) {
        command = argv[i];
        break;
      }
    }
  }

  if (command.empty()) {
    command = "repl";
  }

  int rest_argc = argc - i - 1;

  if (command == "repl") {
    repl(sock);
  } else if (command == "help" || command == "h") {
    usage(argv[0]);
    return 0;
  } else if (command == "version" || command == "v") {
    printf("xbus v%s\n", XBUS_VERSION);
    return 0;
  } else if (command == "list" || command == "l" || command == "ls") {
    auto response = xbus::Response::fromString(sendRequest(sock, "+list"));
    if (response.status == "OK") {
      for (auto& x : response.rest) {
        printf("%s\n", x.c_str());
      }
    } else {
      return 1;
    }
  } else if (command == "call" || command == "c"
          || command == "request" || command == "r"
          || command == "notify" || command == "n") {
    xbus::Request request;

    if (command == "notify" || command == "n") {
      request.action = xbus::ACTION_NOTIFY;
    } else {
      request.action = xbus::ACTION_PROPERTY;
    }

    if (rest_argc >= 2) {
      if (strcmp("-", argv[++i])) {
        request.object = argv[i];
      }
      request.subject = argv[++i];
    }
    
    if (rest_argc == 3) {
      request.args = xbus::splitString(argv[++i], ',');
    }

    if (request.subject.empty()) {
      xbus::error("Usage: call [OBJECT] NAME [ARGS]");
      return 1;
    }

    if (command == "request" || command == "r") {
      request.request = true;
    }

    printf("%s\n", sendRequest(sock, request.toString()).c_str());
  } else if (command == "get") {
    if (rest_argc != 2) {
      xbus::error("Usage: get OBJECT NAME");
      return 1;
    }
    xbus::Request request;
    request.action = xbus::ACTION_FIELD;
    request.request = true;
    request.object = argv[++i];
    request.subject = argv[++i];

    printf("%s\n", sendRequest(sock, request.toString()).c_str());
  } else if (command == "set") {
    if (rest_argc != 3) {
      xbus::error("Usage: get OBJECT NAME VALUE");
      return 1;
    }
    xbus::Request request;
    request.action = xbus::ACTION_FIELD;
    request.object = argv[++i];
    request.subject = argv[++i];
    request.args = {argv[++i]};

    printf("%s\n", sendRequest(sock, request.toString()).c_str());
  } else if (command == "send") {
    if (rest_argc == 1) {
      xbus::error("Usage: send REQUEST\n");
      return 1;
    }
    printf("%s\n", sendRequest(sock, argv[++i]).c_str());
  } else if (command == "wait") {
    xbus::error("Unimplemented");
    return 1;
  } else if (command == "listen") {
    std::string notification;
    if (rest_argc == 1) {
      notification = argv[++i];
    }
    while (1) {
      xbus::Socket socket(sock);
      socket.connect();
      std::string buffer = socket.read(XBUS_READ_SIZE);
      auto request = xbus::Request::fromString(buffer);
      if (request.action == xbus::ACTION_NOTIFY) {
        if (notification.empty()) {
          printf("%s", request.toString().c_str());
          return 0;
        } else if (notification == request.subject) {
          printf("%s", request.toString().c_str());
          return 0;
        }
      }
    }
  } else if (command == "parse_req") {
    std::string what, req;
    if (rest_argc != 2) {
      xbus::error("Usage: parse_req WHAT REQUEST");
      return 1;
    }
    what = argv[++i];
    req = argv[++i];
    auto request = xbus::Request::fromString(req);
    if (!request.isValid()) {
      xbus::error("Request is not valid");
      return 1;
    }
    if (what == "object") {
      printf("%s\n", request.object.c_str());
    } else if (what == "action") {
      printf("%s\n", request.action.c_str());
    } else if (what == "subject") {
      printf("%s\n", request.subject.c_str());
    } else if (what == "request") {
      printf("%d\n", request.request);
    } else if (what == "async") {
      printf("%d\n", request.async);
    } else {
      try {
        int n = std::stoi(what);
        if (n < request.args.size()) {
          printf("%s\n", request.args[n].c_str());
        } else {
          xbus::error("Index out of bounds");
          return 1;
        }
      } catch (...) {
        xbus::error("Usage: parse_req WHAT REQUEST");
        return 1;
      }
    }
  } else if (command == "parse_res") {
    std::string what, req;
    if (rest_argc != 2) {
      xbus::error("Usage: parse_res WHAT RESPONSE");
      return 1;
    }
    what = argv[++i];
    req = argv[++i];
    auto response = xbus::Response::fromString(req);
    if (what == "status") {
      printf("%s\n", response.status.c_str());
    } else {
      try {
        int n = std::stoi(what);
        if (n < response.rest.size()) {
          printf("%s\n", response.rest[n].c_str());
        } else {
          xbus::error("Index out of bounds");
          return 1;
        }
      } catch (...) {
        xbus::error("Usage: parse_res WHAT RESPONSE");
        return 1;
      }
    }
  } else {
    xbus::error("Unknown command: '%s'", command.c_str());
    return 1;
  }

  return 0;
}
