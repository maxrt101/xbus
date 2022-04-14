#include <functional>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <queue>
#include <map>

#include <cstdio>
#include <cstdlib>

#include <signal.h>

#include <xbus/xbus.h>
#include <xbus/utils.h>
#include <xbus/log.h>

#include <mrt/threads/pool.h>
#include <mrt/threads/task.h>
#include <mrt/threads/locked.h>
#include <mrt/threads/future.h>
#include <mrt/container_utils.h>

// For use in handlers of type (Request) -> Response
#define _XBUS_EXPECT_ARGS(n) \
  do { \
    if (request.args.size() != n) {\
      return {"ERR", {"ARGUMENT MISMATCH"}};\
    } \
  } while (0)

#define _XBUS_CHECK_ARGV() \
  do { \
    if (i + 1 >= argc) { \
      xbus::error("'%s' requires an option", argv[i]); \
      usage(argv[0]); \
      return 1; \
    } \
  } while (0)


struct ResponseContext {
  mrt::Future<xbus::Response> response;
  std::atomic<bool> responseInitialized;
  int responseTargetFd = 0;
};

struct ClientContext {
  xbus::Socket* socket = nullptr;
  std::vector<ResponseContext*> responses;
};


static mrt::Locked<std::map<std::string, xbus::Socket*>> g_objects;
static mrt::Locked<std::map<int, ClientContext>> g_clients;


static void sigpipe_handler(int) {
  xbus::warning("SIGPIPE");
}

static void createCtx(xbus::Socket* socket) {
  g_clients.update([socket](auto& clients) {
    clients[socket->fd()].socket = socket;
  });
}

static xbus::Response awaitResponse(xbus::Socket* socket, int targetFd) {
  ResponseContext* ctx = new ResponseContext;

  g_clients.update([socket, ctx](auto& clients) {
    clients[socket->fd()].responses.push_back(ctx);
  });

  xbus::rdebug("[%d]: awaitResponse (%p) fd=%d", socket->fd(), ctx, targetFd);

  ctx->responseTargetFd = targetFd;
  ctx->responseInitialized.store(true);
  xbus::Response response = ctx->response.get();

  xbus::rdebug("[%d]: awaitResponse (%p) fd=%d: got response '%s'", socket->fd(), ctx, targetFd, response.toString().c_str());
  
  g_clients.update([socket, targetFd](auto& clients) {
    clients[socket->fd()].responses.erase(
      std::remove_if(clients[socket->fd()].responses.begin(), clients[socket->fd()].responses.end(),
        [targetFd](auto element) {
          return element->responseTargetFd == targetFd;
        }),
      clients[socket->fd()].responses.end()
    );
  });
  delete ctx;

  return response;
}

static xbus::Response handleBusRequest(xbus::Request request, xbus::Socket* client) {
  xbus::Response response = {"ERR", {"UNKNOWN ACTION"}};
  if (request.action == xbus::ACTION_PROPERTY) {
    if (request.subject == "close") {
      return {""};
    } else if (request.subject == "register") {
      _XBUS_EXPECT_ARGS(1);
      if (g_objects.get().find(request.args[0]) != g_objects.get().end()) {
        response = {"ERR", {"ALREADY REGISTERED", request.args[0]}};
      } else {
        xbus::rinfo("[%d]: register '%s'", client->fd(), request.args[0].c_str());
        g_objects.update([request, client](auto& objects) { objects[request.args[0]] = client; });
        response = {"OK"};
      }
    } else if (request.subject == "version") {
      response = {"OK", {XBUS_VERSION}};
    } else if (request.subject == "list") {
      g_objects.withLocked([&response](auto& objects) {
        std::vector<std::string> objectNames;
        for (auto& p : objects) {
          objectNames.push_back(p.first);
        }
        response = {"OK", objectNames};
      });
    } else if (request.subject == "fd") {
      response = {"OK", {std::to_string(client->fd())}};
    } else {
      response = {"ERR", {"UNKNOWN PROPERTY"}};
    }
  } else if (request.action == xbus::ACTION_NOTIFY) {
    int count = 0;
    g_clients.withLocked([&count, client, request](auto& clients) {
      for (auto& clientCtx : clients) {
        if (clientCtx.second.socket != client) {
          clientCtx.second.socket->write(request.toString() + '\0');
          count++;
        }
      }
    });
    response = {"OK", {"SENT", std::to_string(count)}};
  }
  return response;
}

static void handleRequest(xbus::Request request, xbus::Socket* client) {
  xbus::rinfo("[%d]: recv '%s'", client->fd(), request.toString().c_str());

  xbus::Response response = {"ERR"};
  if (request.object == "") {
    response = handleBusRequest(request, client);
    if (response.status.empty()) {
      return;
    }
  } else {
    xbus::rinfo("[%d]: remote '%s'", client->fd(), request.toString().c_str());
    if (g_objects.get().find(request.object) == g_objects.get().end()) {
      response = {"ERR", {"NO SUCH OBJECT"}};
    } else {
      request.tag = client->fd();
      g_objects.get()[request.object]->write(request.toString() + '\0');
      if (request.action == xbus::ACTION_NOTIFY) {
        response = {"OK", {"SENT"}};
      } else {
        response = awaitResponse(g_objects.get()[request.object], client->fd());
        response.tag = 0;
        xbus::rdebug("[%d]: recv response from %d: '%s'", client->fd(), g_objects.get()[request.object]->fd(), response.toString().c_str());
      }
    }
  }
  client->write(response.toString() + '\0');
}

static void cleanClient(xbus::Socket* client) {
  int fd = client->fd();
  g_objects.withLocked([client](auto& objects) {
    for (auto it = objects.begin(); it != objects.end(); ++it) {
      if (it->second == client) {
        objects.erase(it);
        break;
      }
    }
  });

  g_clients.withLocked([client](auto& clients) {
    auto itr = clients.find(client->fd());
    if (itr != clients.end()) {
      clients.erase(itr);
    }
  });

  delete client;

  xbus::rinfo("[%d] disconnected", fd);
}

static void handleClient(xbus::Socket* client) try {
  createCtx(client);

  std::queue<std::string> requestsQueue;

  std::string buffer;
  while (1) {
    buffer = client->read(XBUS_READ_SIZE);

    if (buffer.empty()) {
      break;
    }

    if (buffer.find('\0') != std::string::npos) {
      auto requests = xbus::splitString(buffer, '\0');
      for (auto r : requests) {
        requestsQueue.push(r);
      }
    } else {
      requestsQueue.push(buffer);
    }

    while (!requestsQueue.empty()) {
      if (!g_clients.get()[client->fd()].responses.empty()) {
        auto response = xbus::Response::fromString(requestsQueue.front());
        xbus::rdebug("[%d] got response '%s'", client->fd(), response.toString().c_str());

        g_clients.update([&requestsQueue, &response, client](auto& clients) {
          auto itr = std::find_if(clients[client->fd()].responses.begin(), clients[client->fd()].responses.end(),
            [&response](auto element) {
              return element->responseTargetFd == response.tag;
            });

          if (itr != clients[client->fd()].responses.end()) {
            (*itr)->response.set(response);
            (*itr)->responseInitialized.store(false);
            requestsQueue.pop();
          } else {
            xbus::rerror("[%d]: unrecognized response '%s', discarding", client->fd(), response.toString().c_str());
            requestsQueue.pop();
          }
        });
        continue;
      }
      if (xbus::isRequest(requestsQueue.front())) {
        xbus::Request request = xbus::Request::fromString(requestsQueue.front());
        requestsQueue.pop();
        handleRequest(request, client);
      } else {
        xbus::rerror("[%d]: unexpected response: '%s', discarding", client->fd(), requestsQueue.front().c_str());
        requestsQueue.pop();
      }
    }
  }

  cleanClient(client);
} catch (xbus::SocketException& e) {
  e.print();
  cleanClient(client);
  return;
} catch (xbus::IOException& e) {
  e.print();
  cleanClient(client);
  return;
}

void usage(const char* argv0) {
  fprintf(stderr,
    "xbusd v%s\n"
    "Usage: %s [OPTIONS]\n"
    "Options:\n"
    "  -h, --help             - Shows this message\n"
    "  -v, --version          - Shows version\n"
    "  -t N, --threads N      - Specify the number of threads (default is equal to hardware concurency)\n"
    "  -s SOCK, --socket SOCK - Unix socket for deamon\n"
    "  -l LVL, --loglevel LVL - Sets log level (debug, info, warning, error, fatal)\n"
    "", XBUS_VERSION, argv0);
}

int main(int argc, char ** argv) {
  signal(SIGPIPE, sigpipe_handler);

  std::string sock = xbus::SOCKET_PATH;
  int threads = 0;

  for (int i = 1; i < argc; i++) {
    if (!strcmp("-v", argv[i]) || !strcmp("--version", argv[i])) {
      printf("xbus v%s\n", XBUS_VERSION);
      return 0;
    } else if (!strcmp("-h", argv[i]) || !strcmp("--help", argv[i])) {
      usage(argv[0]);
      return 0;
    } else if (!strcmp("-t", argv[i]) || !strcmp("--threads", argv[i])) {
      _XBUS_CHECK_ARGV();
      try {
        threads = std::stoi(argv[++i]);
      } catch (...) {
        xbus::error("Invalid number for '%s'", argv[i-1]);
        return 1;
      }
    } else if (!strcmp("-s", argv[i]) || !strcmp("--socket", argv[i])) {
      _XBUS_CHECK_ARGV();
      sock = argv[++i];
    } else if (!strcmp("-l", argv[i]) || !strcmp("--loglevel", argv[i])) {
      _XBUS_CHECK_ARGV();
      xbus::setLogLevel(xbus::stringToLogLevel(argv[++i]));
    } else {
      xbus::error("Unknown argument: '%s'", argv[i]);
      return 1;
    }
  }

  printf("xbus v%s\n", XBUS_VERSION);

  xbus::Socket socket(sock);

  remove(sock.c_str());

  socket.bind();
  socket.listen();

  mrt::ThreadPool<mrt::Task<xbus::Socket*>> pool;

  while (1) {
    xbus::Socket* client = socket.accept();
    if (!client) continue;
    xbus::info("new client: %d", client->fd());
    pool.addTask({handleClient, client});
  }
}
