#include "webserv.hpp"
#include "../includes/CGIHandler.hpp"
#include "../includes/Request.hpp"

CGIHandler::CGIHandler()
{
	_requests.clear();
	_requests.reserve(10);
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

void	CGIHandler::handleChildProcess(s_CGIrequest request)
{
	int pipedf[2] = {request.pipe[0] , request.pipe[1]};

	// Handle pipes
	close(pipedf[READ]); // Closes parent end of pipe
	dup2(pipedf[WRITE], STDOUT_FILENO); // Redirect write end of pipe to STDOUT
	close(pipedf[WRITE]); // Closes write end of pipe (already dupped)

	// set path to executable script
	std::string charPath = "var/www/cgi-bin/example_cgi.py";

	const char * path = charPath.c_str();
	char * const * argv = {nullptr};
	char * const * envp = {nullptr};

	//envp = {nullptr}; // envp should be set according to CGIEnv (const Char*)
	execve(path, argv, envp);

	// Handle execve fail
	throw::std::runtime_error("Execve failed");
	exit(1);
}

void	CGIHandler::handleParentProcess(s_CGIrequest request)
{
	int pipedf[2] = {request.pipe[0] , request.pipe[1]};
	char buffer[1024];
	ssize_t bytesRead;

	close(pipedf[WRITE]);

	while ((bytesRead = read(pipedf[READ], buffer, sizeof(buffer) - 1)) > 0)
	{
		buffer[bytesRead] = '\0';
		request.output += buffer;
	}
	close(pipedf[READ]);
}

void	CGIHandler::runCGIScript(s_client client)
{
	t_CGIrequest request = _requests[client.fd];

	if (request.status == READY)
	{
		if (pipe(request.pipe) < 0){
			throw::std::runtime_error("CGI: Pipe failed"); // handle 500 Internal Server Error
		}
		pid_t pid = fork();
		if (pid < 0){
			throw::std::runtime_error("CGI: Fork"); // handle 500 Internal Server Error
		}
		if (pid == 0)
			handleChildProcess(request);
		else
		{
			request.childPid = pid;
			handleParentProcess(request);
		}
		waitpid(request.childPid, &request.status, 0); // is waitpid necessary
		if (WIFEXITED(request.status))
		{
			// log status and errors
			std::cout << "Child exited with " << WEXITSTATUS(request.status) << "\n";
		}
		else if (WIFSIGNALED(request.status))
		{
			std::cout << "Child process terminated with " << WTERMSIG(request.status) << "\n";
		}

		// send response to client and close
		client.responseString = request.output;
	}
	else if (_requests[client.fd].status == ERROR)
	{
		throw::std::runtime_error("Execve failed");
	}
}

void	CGIHandler::setupCGI(s_client client)
{
	// check that the status of the client is correct
	// add the client to the requests
	if (client.request->method == "GET" && !client.requestReady)
	{
		t_CGIrequest instance;
		// instance.argv.clear();
		instance.CGIEnv.clear();
		instance.status = READY;

		_requests.emplace(client.fd, instance);
		
	}
}