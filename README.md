## Build

### Prerequisites

* SFML2 (libsfml-dev on Debian)
* CMake 3.1 or newer
* GCC or Clang

### Steps

* Generate makefiles with `cmake -G "Unix Makefiles"`
* Build with `make server` and `make client`

## Run

Start server with `server <ip address> <port>`  
Start client with `client <ip address> <port> <nickname>`
