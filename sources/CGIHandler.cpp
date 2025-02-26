#include "webserv.hpp"
#include "../includes/CGIHandler.hpp"
#include "../includes/Request.hpp"

CGIHandler::CGIHandler() : _pipefd({-1, -1}), _output("")
{
	_CGIEnv.clear();
	_argv.clear();
	_envp.clear();
}

void	CGIHandler::setCGIEnv(const Request &request) // takes request
{
	// Parse request and set _CGIEnv:
	// Request method
	// Script name
	// Query string (if applicable, for GET requests)
	// Content length (for post requests)
	// Content type
	// Path info
	// Remote address
	// HTTP_* -> all request headers must be passed as HTTP_HEADER_NAME
}

void	CGIHandler::handleChildProcess()
{
	// Handle pipes
	close(_pipefd[READ]); // Closes parent end of pipe
	dup2(_pipefd[WRITE], STDOUT_FILENO); // Redirect write end of pipe to STDOUT
	close(_pipefd[WRITE]); // Closes write end of pipe (already dupped)

	// set path to executable script
	_argv = {"var/www/cgi-bin/example_cgi.py", nullptr};
	_envp = {nullptr}; // envp should be set according to CGIEnv (const Char*)
	execve(_scriptPath.c_str(), _argv.data(), _envp.data());

	// Handle execve fail
	throw::std::runtime_error("Execve failed");
	exit(1);
}

void	CGIHandler::handleParentProcess()
{
	close(_pipefd[WRITE]);
	// Read output
	char buffer[1024];
	ssize_t bytesRead;

	while ((bytesRead = read(_pipefd[READ], buffer, sizeof(buffer) - 1)) > 0)
	{
		buffer[bytesRead] = '\0';
		_output += buffer;
	}
	close(_pipefd[READ]);
}

std::string	CGIHandler::runCGIScript(const std::string &path, const std::string &body)
{
	int status;

	if (pipe(_pipefd) < 0){
		throw::std::runtime_error("Pipe failed"); // handle 500 Internal Server Error
	}
	pid_t pid = fork();
	if (pid < 0){
		throw::std::runtime_error("Fork"); // handle 500 Internal Server Error
	}
	if (pid == 0)
		handleChildProcess();
	else
		handleParentProcess();
	waitpid(pid, &status, 0);
}