# xbus

Bus architecture for unix-like systems.  
Implements IPC and RPCs.  
Insired by OpenWrt's ubus.  

## How to build
 - Clone repo
 - run `make`
 - run `make install PREFIX=/path/to/installation/folder`

For debugging the build use `make V=1`  
For debugging the runtime use `make DEBUG=1`  
To use in your applications add `-lxbus` to `CFLAGS`

## Example
```C++
#include <xbus/xbus.h>
#include <cstdio>

class Test : public xbus::Object<Test> {
  std::string m_status = "0";

 public:
  Test() : Object("test") {
    addField("value", "0");
    addProperty("status", &Test::status);
  }

  void onNotify(xbus::Request request) override {
    printf("Got notification: %s\n", request.toString().c_str());
  }

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
};

int main() {
  Test test;
  test.listen();

  return 0;
}
```

## Architecture

xbus consists of 3 major components: `xbusd` daemon, `xbus` cli tool and `libxbus`.

### 1. `xbusd` deamon
`xbusd` is a daemon that handles xbus clients and routing of requests and responses. Internally it uses unix domain sockets and custom text-based protocol to send and recieve messages.

#### Usage
```
$ xbusd -h
xbusd v0.1.0
Usage: xbusd [OPTIONS]
Options:
  -h, --help             - Shows this message
  -v, --version          - Shows version
  -t N, --threads N      - Specify the number of threads (default is equal to hardware concurency)
  -s SOCK, --socket SOCK - Unix socket for deamon
  -l LVL, --loglevel LVL - Sets log level (debug, info, warning, error, fatal)
```

### 2. `xbus` command
CLI tool that allows interfacing with xbus. Can be used for testing, or in shell scripts.

#### Usage
```
$ xbus help
xbus v0.1.0
Usage: xbus [OPTIONS] COMMAND [ARGS...]
Commands:
  help                       - Shows this mwssage
  version                    - Show version
  list                       - Shows list of registered objects
  call OBJECT NAME [ARGS]    - Sends a property call
  request OBJECT NAME [ARGS] - Sends a property request
  notify OBJECT NAME [ARGS]  - Sends a notification
  get OBJECT NAME            - Gets a field
  set OBJECT NAME VALUE      - Sets a field
  send REQUEST               - Send raw request
  wait                       - Wait for an object
  listen [OBJECT]            - Listen for notifications
  parse_req WHAT REQUEST     - WHAT can be 'object', 'action', 'subject', 
                               'request', 'async' or number for arg in args
  parse_res WHAT RESPONSE    - WHAT can be 'status' or number for arg in args
Options:
  -s SOCK, --socket SOCK - Unix socket for xbusd
  -l LVL, --loglevel LVL - Sets log level (debug, info, warning, error, fatal)
If COMMAND is empty - REPL will run
To call/notify global property, put '-' instead of OBJECT

```

### 3. `libxbus`
Provides interface for xbus from C++ code. Example can be found above, reference - below

### 4. `xbus` message protocol
Text based protocol built on top of unix domain sockets, but can be used over virtually any protocol stack. Request and reponse defitions are provided using ebnf-like grammar.

```
Legend:
[X]      - optional
X ...    - X followed by more X
[X ...]  - zero or more X
(X)      - group
```

#### Requests
```
format     : [object] action subject [args] [async] [request] [tag]

identifier : [a-zA-z0-9]+
object     : identifier
action     : identifier
subject    : identifier
action     : '-'
           | '+'
           | '!'
args       : ':' arg [, arg ...]
           | '=' arg [, arg ...]
arg        : [^\-+?&#]+
async      : '&'
request    : '?'
tag        : '#' [0-9]+
```

Exaples:  
```
+register:test
test+status?
test+wait:4
```

#### Responses
```
format   : status [rest]
string   : [^,]+
status   : string
rest     : , rest
         | string

```

Exaples:
```
OK
OK,WAIT,3
ERR,ARGUMENT MISMATCH
```

## libxbus reference
`xbus::Object<T>` - Represents an xbus object  
 - `Object(std::string name)` - Constructs and registers an Object. `name` is a xbus object name
 - `addField(std::string field, std::string value)`  - adds a fields
 - `addProperty(std::string prop, HandlerType handler)` - registers a property handler
 - `listen()` - starts listening on the xbus socket
 - `stop()` - stops execution
 - `isRunning() -> bool`
 - `virtual onNotify(Request)` - called when notification comes through

`xbus::Request` - Represents an xbus request   
 - `object: std::string`
 - `action: std::string`
 - `subject: std::string`
 - `args: std::vector<std::string>`
 - `request: bool`
 - `async: bool`
 - `tag: int`
 - `isValid() -> bool`
 - `toString() -> std::string`
 - `static fromString(std::string str) -> Request`

`xbus::Response` - Represents an xbus response  
 - `status: std::string`
 - `rest: std::vector<std::string>`
 - `tag: int`
 - `Response(std::string status)`
 - `Response(std::string status, std::vector<std::string> rest)`
 - `toString() -> std::string`
 - `static fromString(std::string str) -> Response`

`xbus::Socket` - Abstraction for unix socket (basically wraps file descriptor)  
 - `Socket(const std::string& path)` - Creates a unix socket
 - `path() -> std::string ` - returns path
 - `fd() -> int` - returns file descriptor (`-1` if socket is invalid)
 - `bind()` - binds socket to path
 - `connect()` - connects socket to path
 - `listen()` - listens for incoming connections
 - `close()` - closes the socket
 - `accept() -> Socket*` - accepts new client and returns new Socket for him (returned socket must be freed)
 - `write(std::string data)` - writes a sting to the socket
 - `read(size_t size) -> std::string ` - reads size bytes from socket (will block, until data is present)

`xbus::IOException` - Gets throws when `read` or `write` fail  

`xbus::SocketException` - Gets thrown when socket operations fail (`listen`, `connect`, etc)  
