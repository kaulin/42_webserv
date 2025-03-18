#include "webserv.hpp"
#include "../includes/CGIHandler.hpp"
#include "../includes/Request.hpp"

CGIHandler::CGIHandler() : _output("")
{
	_CGIEnv.clear();
	_argv.clear();
	_envp.clear();
}

void	CGIHandler::setCGIEnv(const HttpRequest &request) // takes request
{
	std::cout << "Setting CGI env:" << request.uri << "\n";
	// Parse request and set _CGIEnv with setenv
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
	//std::string path = "var/www/cgi-bin/example_cgi.py";
	std::string charPath = "var/www/cgi-bin/example_cgi.py";

	_argv.emplace_back(charPath.c_str());
	_argv.emplace_back(nullptr);

	_envp = {nullptr}; // envp should be set according to CGIEnv (const Char*)
	execve(_scriptPath.c_str(), _argv.data(), _envp.data());

	// Handle execve fail
	throw::std::runtime_error("Execve failed");
	exit(1);
}

void	CGIHandler::handleParentProcess()
{
	char buffer[1024];
	ssize_t bytesRead;

	close(_pipefd[WRITE]);

	while ((bytesRead = read(_pipefd[READ], buffer, sizeof(buffer) - 1)) > 0)
	{
		buffer[bytesRead] = '\0';
		_output += buffer;
	}
	close(_pipefd[READ]);
}

void	CGIHandler::runCGIScript(const std::string &path, const std::string &requestBody)
{
	int status;
	//	Response response; The response that should be returned to the client

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
	{
		_childPid = pid;
		handleParentProcess();
	}
	waitpid(_childPid, &status, 0); // is waitpid necessary
	if (WIFEXITED(status))
	{
		// log status and errors
		std::cout << "Child exited with " << WEXITSTATUS(status) << "\n";
	}
	else if (WIFSIGNALED(status))
	{
		std::cout << "Child process terminated with " << WTERMSIG(status) << "\n";
	}
	// send response to client and close
}

std::string	CGIHandler::getCGIOutput() { return _output; }