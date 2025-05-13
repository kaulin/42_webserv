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
  - implement CGI with a chosen language
  - serve fully static websites and work with a chosen browser
  - be able to listen to multiple ports
  - provide accurate HTTP status codes and implement default error pages

## Installation

Git clone the repository and run `make` to compile the program.

## Usage

`./webserv [config file path]`

Some example configuration files and an example site demonstrating the program's features are provided in the repository. Running the program without arguments uses the default configuration file. Configuration file syntax is similar to, but not one-to-one compatible with nginx.

## Authors

[Jaakko Junttila](https://github.com/kaulin), [Erno Michelsson](https://github.com/ernobyl) & [Katja Toivola](https://github.com/kootee)

## Acknowledgements

- Peer evaluators: [Nick Saveliev](https://github.com/FPyMEHTAPIU), Hoang Tran & Eromon Agbomeirele

## Resources
Gundavaram, S. (1996). CGI Programming on the World Wide Web.  
https://www.oreilly.com/openbook/cgi/

InetDaemon (2018, May 19). TCP 3-Way Handshake (SYN,SYN-ACK,ACK).  
https://www.inetdaemon.com/tutorials/internet/tcp/3-way_handshake.shtml

Linux Today (2019, May 9). Blocking and Non-Blocking I/0 Tutorial.  
https://www.linuxtoday.com/blog/blocking-and-non-blocking-i-0/

Matariya, R. (2020, March 2). Beginner’s Guide to NGINX Configuration Files.  
https://medium.com/adrixus/beginners-guide-to-nginx-configuration-files-527fcd6d5efd

Mozilla Corporation. HTTP.  
https://developer.mozilla.org/en-US/docs/Web/HTTP

Fielding, R. & et al. (1999, June). Hypertext Transfer Protocol -- HTTP/1.1.  
https://www.rfc-editor.org/rfc/rfc2616.txt

Thien, T. V. (2020, August 23). A simple HTTP server from scratch.  
https://trungams.github.io/2020-08-23-a-simple-http-server-from-scratch/

Yu, J. (2023, May 1). How I Built a Simple HTTP Server from Scratch using C.  
https://dev.to/jeffreythecoder/how-i-built-a-simple-http-server-from-scratch-using-c-739
