#include "webserv.hpp"
#include "../includes/CGIHandler.hpp"
#include "../includes/Request.hpp"

CGIHandler::CGIHandler()
{
	_requests.clear();
	_requests.reserve(10);
}

std::vector<std::string>	CGIHandler::setCGIEnv(HttpRequest& request) // takes request
{
	std::vector<std::string> env;

	std::cout << "Request headers\n";
	for (const auto &it : request.headers)
	{
		std::cout << it.first << " " << it.second << "\n";
	}

	// Replace temp data with get methods
	env.emplace_back("REQUEST_METHOD=" + request.method);
	if (request.method == "POST")
	{
		env.emplace_back("CONTENT_TYPE=application/x-www-form-urlencoded");
		env.emplace_back("CONTENT_LENGTH="); // needs method to search by header
	}
	env.emplace_back("QUERY_STRING="); 	// (if applicable, for GET requests) needs get method
	env.emplace_back("PATH_INFO=" + request.uri); // needs get method
	env.emplace_back("SERVER_NAME=" + request.headers["host"]);
	env.emplace_back("SERVER_PORT=8080"); 	// needs get method
	env.emplace_back("REMOTE_ADDR=");	// needs get method

	return env;
}

void	CGIHandler::handleChildProcess(s_CGIrequest cgiRequest, s_client client)
{
	int pipedf[2] = {cgiRequest.pipe[0] , cgiRequest.pipe[1]};

	std::cout << "Dup2 pipe write end to stdout" << "path" << "\n";
	// Redirect WRITE end of pipe to STDOUT
	if (dup2(pipedf[WRITE], STDOUT_FILENO) == -1)
	{
		close(client.fd);
		close(pipedf[WRITE]);
		std::exit(EXIT_FAILURE);
	}
	
	close(pipedf[READ]); // Closes parent end of pipe
	close(pipedf[WRITE]); // Closes (already dupped) write end of pipe

	// Set path to executable
	// std::string charPath = "var/www/cgi-bin/example_cgi.py";
	std::vector<char*> argv;
	argv.emplace_back(const_cast<char *>(cgiRequest.CGIPath.c_str()));
	argv.emplace_back(nullptr);

	std::vector<char*> envp;
	std::cout << "Setting cgi env\n";
	for (const auto & var : cgiRequest.CGIEnv)
	{
		std::cout << var << "\n";
		envp.emplace_back(var);
	}
	envp.emplace_back(nullptr);

	std::cout << "Child executing path: " << cgiRequest.CGIPath << "\n";
	execve(cgiRequest.CGIPath.c_str(), argv.data(), envp.data());

	// Handle execve fail
	throw::std::runtime_error("Child: Execve failed");
	std::exit(EXIT_FAILURE);
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
		std::cout << "CGI read: " << buffer << "\n";
	}
	close(pipedf[READ]);
}

void	CGIHandler::runCGIScript(s_client client)
{
	t_CGIrequest request = _requests[client.fd]; // gets the client request from container

	if (request.status == CGI_READY)
	{
		if (pipe(request.pipe) < 0){
			throw::std::runtime_error("CGI: Pipe failed"); // handle 500 Internal Server Error
		}
		pid_t pid = fork();
		if (pid < 0){
			throw::std::runtime_error("CGI: Fork"); // handle 500 Internal Server Error
		}
		if (pid == 0)
			handleChildProcess(request,client);
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
	else if (_requests[client.fd].status == CGI_ERROR)
	{
		throw::std::runtime_error("Execve failed");
	}
}

void	CGIHandler::setupCGI(s_client &client)
{
	// check that the status of the client is correct
	// add the client to the requests
	if (client.requestReady)
	{
		t_CGIrequest cgiInstance;
		cgiInstance.CGIEnv.clear();
		cgiInstance.status = CGI_READY;
		cgiInstance.CGIPath = client.request->uri;
		cgiInstance.CGIEnv = setCGIEnv(*client.request);

		_requests.emplace(client.fd, cgiInstance);
		//_requests.insert({client.fd, instance});
	}
}