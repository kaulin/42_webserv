#include <sys/wait.h>
#include <sys/stat.h>
#include <exception>
#include <iostream>
#include <filesystem>
#include "CGIHandler.hpp"
#include "HttpRequest.hpp"

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

std::vector<char*>	CGIHandler::setCGIEnv(const HttpRequest& request, const Client& client) // takes request
{
	std::vector<std::string> strEnv;

	strEnv.emplace_back("REQUEST_METHOD=" + request.method);
	strEnv.emplace_back("SERVER_NAME=" + request.headers.at("Host"));
	if (request.method == "POST")
	{
		strEnv.emplace_back("CONTENT_TYPE=application/x-www-form-urlencoded");
		strEnv.emplace_back("CONTENT_LENGTH="); // needs method to search by header
	}
	strEnv.emplace_back("QUERY_STRING=" + request.uriQuery);
	strEnv.emplace_back("PATH_INFO=" + request.uri);
	strEnv.emplace_back("SERVER_PORT=" + client.serverConfig->_port);
	strEnv.emplace_back("REMOTE_ADDR=");	// not necessarily needed

	// cast strings to <char *> for correct datatype
	std::vector<char*> env;
	for (const auto &var : strEnv)
	{
		env.emplace_back(const_cast<char *>(var.c_str()));
	}
	env.emplace_back(nullptr);
	return env;
}

void	CGIHandler::handleChildProcess(int clientFd)
{
	t_CGIrequest cgiRequest = _requests[clientFd];
	int pipedf[2] = {cgiRequest.pipe[0], cgiRequest.pipe[1]};
	
	// Redirect WRITE end of pipe to STDOUT
	if (dup2(pipedf[WRITE], STDOUT_FILENO) == -1)
	{
		closeFds({clientFd, pipedf[WRITE]});
		std::exit(EXIT_FAILURE);
	}
	close(pipedf[READ]); // Closes parent end of pipe
	close(pipedf[WRITE]); // Closes (already dupped) write end of pipe

	// Checks that file in CGIPath exists and is executable
	struct stat buff;
	if (stat(cgiRequest.CGIPath.c_str(), &buff) != 0)
	{
		throw::std::runtime_error("Child: File not found");
		std::exit(EXIT_FAILURE);
	}
	if (!(buff.st_mode & S_IXUSR))
	{
		throw::std::runtime_error("Child: File is not executable");
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

void	CGIHandler::runCGIScript(Client& client)
{
	if (_requests[client.fd].status == CGI_READY)
	{
		if (pipe(_requests[client.fd].pipe) < 0){
			throw::std::runtime_error("CGI: Pipe failed"); // handle 500 Internal Server Error
		}
		pid_t pid = fork();
		if (pid < 0){
			throw::std::runtime_error("CGI: Fork"); // handle 500 Internal Server Error
		}
		if (pid == 0)
			handleChildProcess(client.fd);
		else
		{
			// check that child has exited correctly
			// handleParentProcess(_requests[client.fd]);

			close(_requests[client.fd].pipe[WRITE]);

			_requests[client.fd].status = CGI_FORKED;
			_requests[client.fd].childPid = pid;
			client.fileReadFd = _requests[client.fd].pipe[READ];

		}
		// waitpid(_requests[client.fd].childPid, &_requests[client.fd].status, 0); // is waitpid necessary
		// if (WIFEXITED(_requests[client.fd].status))
		// {
		// 	std::cout << "Child exited with " << WEXITSTATUS(_requests[client.fd].status) << "\n";
		// }
		// else if (WIFSIGNALED(_requests[client.fd].status))
		// {
		// 	std::cout << "Child process terminated with " << WTERMSIG(_requests[client.fd].status) << "\n";
		// }
		client.requestReady = true;
		// send response to client and close -- > write to the client
	}
	else if (_requests[client.fd].status == CGI_ERROR)
	{
		throw::std::runtime_error("CGI error");
	}
	_requests.erase(client.fd);
}

void	CGIHandler::setupCGI(Client &client)
{
	// checks that the status of the client is correct
	// needs to check if child is already executing and if it is do nothing
	// add the client to the requests if
	const HttpRequest&  request = client.requestHandler->getRequest();
	
	if (_requests.find(client.fd) != _requests.end())
		return;
	if (client.requestReady && this->_requests.size() < 10)
	{
		const HttpRequest&  request = client.requestHandler->getRequest();
		t_CGIrequest cgiInst;
		cgiInst.CGIPath = std::filesystem::current_path().string() + request.uri;
		cgiInst.argv.emplace_back(const_cast<char *>(cgiInst.CGIPath.c_str()));
		cgiInst.argv.emplace_back(nullptr);
		cgiInst.status = CGI_READY;
		cgiInst.envp.clear();
		cgiInst.envp = setCGIEnv(request, client);
		_requests.emplace(client.fd, cgiInst);
	}
	else
	{
		std::cout << "Server is busy with too many CGI requests, try again in a moment\n";
	}
}