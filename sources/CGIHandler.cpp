#include <sys/wait.h>
#include <sys/stat.h>
#include <exception>
#include <iostream>
#include <filesystem>
#include <fcntl.h>
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

std::string CGIHandler::setCgiPath(const HttpRequest& request)
{
	std::string parsedUri = request.uri;
	if (!request.uriQuery.empty() && parsedUri.find('?') != std::string::npos)
	{
		parsedUri = request.uri.substr(0, request.uri.find('?'));
	}
	std::string cgiUri = std::filesystem::current_path().string() + "/var/www" + parsedUri;
	return cgiUri;
}

std::vector<std::string>	CGIHandler::setCGIEnv(const HttpRequest& request, const Client& client) // takes request
{
	std::vector<std::string>	strEnv;

	strEnv.emplace_back("REQUEST_METHOD=" + request.method);
	strEnv.emplace_back("SERVER_NAME=" + request.headers.at("Host"));
	if (request.method == "POST")
	{
		strEnv.emplace_back("CONTENT_TYPE=application/x-www-form-urlencoded");
		auto contentLength = request.headers.find("Content-Length");
		if (contentLength != request.headers.end())
			strEnv.emplace_back("CONTENT_LENGTH=" + request.headers.at("Content-Length"));
	}
	strEnv.emplace_back("QUERY_STRING=" + request.uriQuery);
	strEnv.emplace_back("PATH_INFO=" + request.uri);
	strEnv.emplace_back("SERVER_PORT=" + client.serverConfig->port);
	strEnv.emplace_back("REMOTE_ADDR=");	// not necessarily needed

	return strEnv;
}

int	CGIHandler::setupCGI(Client &client)
{
	const HttpRequest& request = client.requestHandler->getRequest();
	
	if (!_requests.empty() && _requests.find(client.fd) != _requests.end())
	{
		return (-1);
	}
	if (this->_requests.size() >= 10)
	{
		std::cout << "Server is busy with too many CGI requests, try again in a moment\n";
		return (-1);
	}
	std::unique_ptr<t_CGIrequest> cgiInst = std::make_unique<t_CGIrequest>();

	cgiInst->CGIPath = setCgiPath(request);
	cgiInst->argv.emplace_back(const_cast<char *>(cgiInst->CGIPath.c_str()));
	cgiInst->argv.emplace_back(nullptr);
	cgiInst->requestBody = request.body;
	cgiInst->status = CGI_READY;

	// Prepare pipes
	if (pipe(_requests[client.fd]->inPipe) < 0 || pipe(_requests[client.fd]->outPipe))
	{
		throw ServerException(STATUS_INTERNAL_ERROR); // fatal
	}

	int flags;

	// Get current file status flags and add O_NONBLOCK to the flags
	if ((flags = fcntl(_requests[client.fd]->inPipe[WRITE], F_GETFL)) == -1)
	{
		throw ServerException(STATUS_INTERNAL_ERROR); // fatal
	}
	if (fcntl(_requests[client.fd]->inPipe[WRITE], F_SETFL, flags | O_NONBLOCK) == -1) 
	{
		throw ServerException(STATUS_INTERNAL_ERROR); // fatal
	}

	_requests.emplace(client.fd, std::move(cgiInst));

	return (_requests[client.fd]->inPipe[WRITE]);
}

void CGIHandler::handleParentProcess(Client& client)
{
	int inPipe[2] = {_requests[client.fd]->inPipe[0], _requests[client.fd]->inPipe[1]};
	int outPipe[2] = {_requests[client.fd]->outPipe[0], _requests[client.fd]->outPipe[1]};
	
	std::string requestBody = _requests[client.fd]->requestBody;

	// Write the request body for the child to read and close unnecessary pipes
	close(outPipe[WRITE]);
	close(inPipe[READ]);
	if (requestBody.length() > 0)
	{
		int bytesWritten = write(inPipe[WRITE], requestBody.c_str(), requestBody.size());
		if (bytesWritten == -1)
		{
			perror("Write failed");
			throw ServerException(STATUS_INTERNAL_ERROR); // fatal
		}
	}
	close(inPipe[WRITE]);

	client.resourceReadFd = dup(outPipe[READ]); // Dup read end to client
	close(outPipe[READ]); // Close dupped read end
}

void	CGIHandler::handleChildProcess(Client& client)
{
	int clientFd = client.fd;

	std::vector<std::string> cgiStrEnv = setCGIEnv(client.requestHandler->getRequest(), client);

	t_CGIrequest cgiRequest = *_requests[clientFd];
	int inPipe[2] = {cgiRequest.inPipe[0], cgiRequest.inPipe[1]};
	int outPipe[2] = {cgiRequest.outPipe[0], cgiRequest.outPipe[1]};
	
	// Setup STDIN to write the necessary data to inpipe READ --> cgi script
	if (dup2(inPipe[READ], STDIN_FILENO) == -1)
	{
		closeFds({clientFd, outPipe[WRITE], outPipe[READ], inPipe[WRITE]});
		std::exit(EXIT_FAILURE);
	}
	close(inPipe[WRITE]);

	// Setup STDOUT to write to pipe WRITE end of outpipe
	if (dup2(outPipe[WRITE], STDOUT_FILENO) == -1)
	{
		closeFds({clientFd, outPipe[READ], inPipe[READ], inPipe[WRITE]});
		std::exit(EXIT_FAILURE);
	}
	close(outPipe[READ]);

	// Checks that file in CGIPath exists and is executable
	struct stat buff;
	if (stat(cgiRequest.CGIPath.c_str(), &buff) != 0)
	{
		perror("Child: File not found");
		closeFds({clientFd, inPipe[READ], outPipe[WRITE]});
		std::exit(EXIT_FAILURE);
	}
	if (!(buff.st_mode & S_IXUSR))
	{
		perror("Child: File is not executable");
		closeFds({clientFd, inPipe[READ], outPipe[WRITE]});
		std::exit(EXIT_FAILURE);
	}

	// Set environment variables for execve call
	std::vector<char*> envp;
	for (const auto &var : cgiStrEnv)
	{
		envp.emplace_back(const_cast<char *>(var.c_str()));
	}
	envp.emplace_back(nullptr);

	execve(cgiRequest.CGIPath.c_str(), cgiRequest.argv.data(), envp.data());

	perror("Child: Execve failed");
	std::exit(EXIT_FAILURE);
}

void	CGIHandler::runCGIScript(Client& client)
{
	// setupCGI(client);

	if (_requests[client.fd]->status != CGI_READY)
		return;
	
	pid_t pid = fork();
	if (pid < 0)
	{
		kill(pid, SIGTERM);
		_requests[client.fd]->status = CGI_ERROR;
		throw ServerException(STATUS_INTERNAL_ERROR); // fatal
	}
	if (pid == 0)
	{
		handleChildProcess(client);
	}
	else
	{
		handleParentProcess(client);
	}
	_requests[client.fd]->status = CGI_FORKED;
	_requests[client.fd]->childPid = pid;

	if (_requests[client.fd]->status == CGI_ERROR)
	{
		throw ServerException(STATUS_INTERNAL_ERROR);
	}
	_requests.erase(client.fd);
}
