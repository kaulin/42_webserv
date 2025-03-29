#include <sys/wait.h>
#include <sys/stat.h>
#include <exception>
#include <iostream>
#include "ServerHandler.hpp"
#include "CGIHandler.hpp"
#include <filesystem>

CGIHandler::CGIHandler()
{
	_requests.clear();
	_requests.reserve(10);
}

void CGIHandler::closeFds(const std::vector<int> fdsToclose)
{
	for (const auto & fd : fdsToclose)
	{
		close(fd);
	}
}

std::vector<std::string>	CGIHandler::initCGIEnv(HttpRequest& request) // takes request
{
	std::vector<std::string> env;
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

void CGIHandler::setCGIEnv(t_CGIrequest &cgiRequest, std::vector<char *> &envp)
{
	for (const auto &var : cgiRequest.CGIEnv)
	{
		envp.emplace_back(const_cast<char *>(var.c_str()));
	}
	envp.emplace_back(nullptr);
}

void	CGIHandler::handleChildProcess(t_CGIrequest cgiRequest, Client& client)
{
	int pipedf[2] = {cgiRequest.pipe[0], cgiRequest.pipe[1]};
	std::vector<char*> argv;
	std::vector<char*> envp;

	// Redirect WRITE end of pipe to STDOUT
	if (dup2(pipedf[WRITE], STDOUT_FILENO) == -1)
	{
		closeFds({client.fd, pipedf[WRITE]});
		std::exit(EXIT_FAILURE);
	}
	
	close(pipedf[READ]); // Closes parent end of pipe
	close(pipedf[WRITE]); // Closes (already dupped) write end of pipe

	setCGIEnv(cgiRequest, envp);
	argv.emplace_back(const_cast<char *>(cgiRequest.CGIPath.c_str()));
	argv.emplace_back(nullptr);

	struct stat buff;
	// Checks that file in CGIPath exists
	if (stat(cgiRequest.CGIPath.c_str(), &buff) != 0)
	{
		perror("File not found");
		std::exit(EXIT_FAILURE);
	}
	if (!(buff.st_mode & S_IXUSR))
	{
		perror("File is not executable");
		std::exit(EXIT_FAILURE);
	}

	execve(cgiRequest.CGIPath.c_str(), argv.data(), envp.data());

	perror("execve failed");
	throw::std::runtime_error("Child: Execve failed");
	std::exit(EXIT_FAILURE);
}

void CGIHandler::handleParentProcess(t_CGIrequest request)
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
	//close(pipedf[READ]);
}

void	CGIHandler::runCGIScript(Client& client)
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
			handleChildProcess(request, client);
		else
		{
			
			request.status = CGI_FORKED;
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

		// send response to client and close -- > write to the client
		client.responseBodyString = request.output;
	}
	else if (_requests[client.fd].status == CGI_ERROR)
	{
		throw::std::runtime_error("Execve failed");
	}
}

std::string	CGIHandler::setCGIPath(std::string uri)
{
	std::string path = std::filesystem::current_path().string() + uri;
	std::cout << "CGI path " << path << "\n";
	return path;
}

void	CGIHandler::setupCGI(Client &client)
{
	// check that the status of the client is correct
	// add the client to the requests
	if (client.requestReady)
	{
		t_CGIrequest cgiInstance;
		cgiInstance.CGIEnv.clear();
		cgiInstance.status = CGI_READY;
		cgiInstance.CGIPath = setCGIPath(client.request->uri);
		cgiInstance.CGIEnv = initCGIEnv(*client.request);
		_requests.emplace(client.fd, cgiInstance);
	}
}