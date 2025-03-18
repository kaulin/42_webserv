#pragma once

#include "webserv.hpp"
#include "Request.hpp"

#define READ 0
#define WRITE 1
#define PIPE_LIMIT 65536;

class Request;

class CGIHandler {
private:
	std::string					_scriptPath;
	std::vector<std::string>	_CGIEnv;
	int							_pipefd[2];
	int							_childPid;
	std::vector<char* const>	_argv; // args for execve call
	std::vector<char*>			_envp;
	std::string					_output;

	// Private class methods
	void	handleChildProcess();
	void	handleParentProcess();
public:
	CGIHandler();
	void			setCGIEnv(const HttpRequest &request);
	void			runCGIScript(const std::string &path, const std::string &body);
	std::string		getCGIOutput();
};
