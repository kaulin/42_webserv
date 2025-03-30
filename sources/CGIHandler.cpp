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

std::vector<char*>	CGIHandler::setCGIEnv(const HttpRequest& request) // takes request
{
	std::vector<char*> env;

	env.emplace_back(const_cast<char*>(("REQUEST_METHOD=" + request.method).c_str()));
	if (request.headers.find("Host") != request.headers.end())
		env.emplace_back(const_cast<char*>(("SERVER_NAME=" + request.headers.at("Host")).c_str()));
	if (request.method == "POST")
	{
		env.emplace_back(const_cast<char*>("CONTENT_TYPE=application/x-www-form-urlencoded"));
		env.emplace_back(const_cast<char*>("CONTENT_LENGTH=")); // needs method to search by header
	}
	env.emplace_back(const_cast<char*>(("QUERY_STRING=" + request.uriQuery).c_str())); 	// (if applicable, for GET requests) needs get method
	env.emplace_back(const_cast<char*>(("PATH_INFO=" + request.uri).c_str()));
	env.emplace_back(const_cast<char*>(("SERVER_PORT=8080"))); 	// needs get method
	env.emplace_back(const_cast<char*>("REMOTE_ADDR="));	// not necessarily needed

	return env;
}

void	CGIHandler::handleChildProcess(t_CGIrequest cgiRequest, int clientFd)
{
	int pipedf[2] = {cgiRequest.pipe[0], cgiRequest.pipe[1]};
	
	// Redirect WRITE end of pipe to STDOUT
	if (dup2(pipedf[WRITE], STDOUT_FILENO) == -1)
	{
		closeFds({clientFd, pipedf[WRITE]});
		std::exit(EXIT_FAILURE);
	}
	close(pipedf[READ]); // Closes parent end of pipe
	close(pipedf[WRITE]); // Closes (already dupped) write end of pipe

	// Checks that file in CGIPath exists
	struct stat buff;
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

	execve(cgiRequest.CGIPath.c_str(), cgiRequest.argv.data(), cgiRequest.envp.data());

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

void	CGIHandler::runCGIScript(const Client& client)
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
			handleChildProcess(request, client.fd);
		else
		{
			request.status = CGI_FORKED;
			request.childPid = pid;
			handleParentProcess(request);
		}
		waitpid(request.childPid, &request.status, 0); // is waitpid necessary
		if (WIFEXITED(request.status))
		{
			std::cout << "Child exited with " << WEXITSTATUS(request.status) << "\n";
		}
		else if (WIFSIGNALED(request.status))
		{
			std::cout << "Child process terminated with " << WTERMSIG(request.status) << "\n";
		}

		// send response to client and close -- > write to the client
		// delete the client
	}
	else if (_requests[client.fd].status == CGI_ERROR)
	{
		throw::std::runtime_error("Execve failed");
	}
}

void	CGIHandler::setupCGI(const Client &client)
{
	// checks that the status of the client is correct
	// add the client to the requests if 
	if (client.requestReady && this->_requests.size() < 10)
	{
		t_CGIrequest cgiInst;
		cgiInst.CGIPath = std::filesystem::current_path().string() + client.request->uri;
		cgiInst.argv.emplace_back(const_cast<char *>(cgiInst.CGIPath.c_str()));
		cgiInst.argv.emplace_back(nullptr);
		cgiInst.status = CGI_READY;
		cgiInst.envp.clear();
		cgiInst.envp = setCGIEnv(*client.request);
		_requests.emplace(client.fd, cgiInst);
	}
	else
	{
		std::cout << "Server is busy with too many CGI requests, try again in a moment\n";
	}
}