#include <sys/wait.h>
#include <sys/stat.h>
#include <exception>
#include <iostream>
#include <filesystem>
#include "CGIHandler.hpp"
#include "HttpRequest.hpp"
#include "ServerException.hpp"

CGIHandler::CGIHandler()
{
	_requests.reserve(10);
}

void CGIHandler::closeFds(const std::vector<int> fdsToclose)
{
	for (const auto & fd : fdsToclose)
		close(fd);
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
	t_CGIrequest cgiRequest = *_requests[clientFd];
	int pipedf[2] = {cgiRequest.pipe[0], cgiRequest.pipe[1]};
	
	// Redirect WRITE end of pipe to STDOUT
	if (dup2(pipedf[WRITE], STDOUT_FILENO) == -1)
	{
		closeFds({clientFd, pipedf[WRITE]});
		std::exit(EXIT_FAILURE);
	}
	close(pipedf[READ]); // Closes parent end of pipe

	// Checks that file in CGIPath exists and is executable
	struct stat buff;
	if (stat(cgiRequest.CGIPath.c_str(), &buff) != 0)
	{
		closeFds({clientFd, pipedf[WRITE]});
		perror("Child: File not found");
		std::exit(EXIT_FAILURE);
	}
	if (!(buff.st_mode & S_IXUSR))
	{
		closeFds({clientFd, pipedf[WRITE]});
		perror("Child: File is not executable");
		std::exit(EXIT_FAILURE);
	}
	execve(cgiRequest.CGIPath.c_str(), cgiRequest.argv.data(), cgiRequest.envp.data());

	perror("Child: Execve failed");
	std::exit(EXIT_FAILURE);
}

void	CGIHandler::setupCGI(Client &client)
{
	const HttpRequest&  request = client.requestHandler->getRequest();
	
	if (!_requests.empty() && _requests.find(client.fd) != _requests.end())
	{
		return;
	}
	if (this->_requests.size() >= 10)
	{
		std::cout << "Server is busy with too many CGI requests, try again in a moment\n";
		return;
	}
	std::unique_ptr<t_CGIrequest> cgiInst = std::make_unique<t_CGIrequest>();

	cgiInst->CGIPath = std::filesystem::current_path().string() + request.uri;
	cgiInst->argv.emplace_back(const_cast<char *>(cgiInst->CGIPath.c_str()));
	cgiInst->argv.emplace_back(nullptr);
	cgiInst->envp = setCGIEnv(request, client);
	cgiInst->status = CGI_READY;
	_requests.emplace(client.fd, std::move(cgiInst));
}

void	CGIHandler::runCGIScript(Client& client)
{
	setupCGI(client);

	if (_requests[client.fd]->status == CGI_READY)
	{
		if (pipe(_requests[client.fd]->pipe) < 0)
		{
			throw ServerException(STATUS_INTERNAL_ERROR);
		}
		pid_t pid = fork();
		if (pid < 0)
		{
			throw ServerException(STATUS_INTERNAL_ERROR);
		}
		if (pid == 0)
		{
			handleChildProcess(client.fd);
		}
		else
		{
			close(_requests[client.fd]->pipe[WRITE]); // Close child end
			client.fileReadFd = dup(_requests[client.fd]->pipe[READ]); // Dup read end to client
			close(_requests[client.fd]->pipe[READ]); // Close dupped read end
		}
		_requests[client.fd]->status = CGI_FORKED;
		_requests[client.fd]->childPid = pid;
	}
	else if (_requests[client.fd]->status == CGI_ERROR)
	{
		throw ServerException(STATUS_INTERNAL_ERROR);
	}
}
