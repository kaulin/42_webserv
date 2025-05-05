# webserv

> [!WARNING]
> To anyone in general, but fellow 42 students in particular: when faced with a problem, it is always better to attempt solving it first alone without consulting the previous works of others. Your precursors were no smarter than and just as fallible as you; Blindly following their footsteps will only lead you on the same detours they took and prevent you from seeing new, faster routes along the way.

## Introduction

This was a 42 cursus project focusing on creating an HTTP server using C++17.

## Project requirements

- Use of external or Boost libraries is not allowed
- Any read and write operations (aside from reading the config) must only be done after going through poll
- Checking errno after read or write operations is forbidden
- The server should:
  - never block and requests should never hang indefinitely
  - support at least GET, POST and DELETE requests
  - implement CGI with a chosen language (we chose Python)
  - serve fully static websites and work with a chosen browser (we chose Firefox)
  - be able to listen to multiple ports
  - provide accurate HTTP status codes and implement default error pages

## Installation

Git clone the repository and run `make` to compile the program.

## Usage

`./webserv [config file path]`

Some example configuration files and an example site demonstrating the servers features are provided in the repository. Running the program without arguments uses the default configuration file. Configuration file syntax is similar to, but not one-to-one compatible with nginx.

## Problems encountered / key takeaways / implementation choises / room for improvement / ???

## Authors

[Jaakko Junttila](https://github.com/kaulin), [Erno Michelsson](https://github.com/ernobyl) & [Katja Toivola](https://github.com/kootee)

## Acknowledgements

- Peer evaluators: [XXX](https://github.com/XXX), [YYY](https://github.com/YYY), Arttu Salo, [ZZZ](https://github.com/ZZZ)

## Resources
