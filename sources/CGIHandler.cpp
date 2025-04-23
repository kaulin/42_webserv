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

CGIHandler::~CGIHandler() {};

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

void	CGIHandler::checkProcesses(int clientFd, pid_t pid)
{
	int i = clientFd;
	for (const auto& it : _pids)
	{
		if (it == pid)
			_pids.erase(_pids.begin() + i);
		i++;
	}
	_requests.erase(clientFd);
}

bool	CGIHandler::readyForExecve(const Client& client)
{
	if (!_requests.empty() && _requests.find(client.fd) != _requests.end())
	{
		auto it = _requests.find(client.fd);
		if (it->second->postMethod && it->second->status == CGI_EXECVE_READY
			&& client.resourceWriteFd == -1) // client has written
		{
			return true;
		}
		else if (!it->second->postMethod && it->second->status == CGI_EXECVE_READY)
			return true;
	}
	return false;
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

void	CGIHandler::setupCGI(Client &client)
{
	const HttpRequest& request = client.requestHandler->getRequest();
	
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

	// Get current file status flags and add O_NONBLOCK
	int flags;
	if ((flags = fcntl(_requests[client.fd]->inPipe[WRITE], F_GETFL)) == -1)
	{
		throw ServerException(STATUS_INTERNAL_ERROR); // fatal
	}
	if (fcntl(_requests[client.fd]->inPipe[WRITE], F_SETFL, flags | O_NONBLOCK) == -1) 
	{
		throw ServerException(STATUS_INTERNAL_ERROR); // fatal
	}

	_requests.emplace(client.fd, std::move(cgiInst));
	client.fileWriteFd = _requests[client.fd]->inPipe[WRITE];
}

void CGIHandler::handleParentProcess(Client& client)
{
	t_CGIrequest cgiRequest = *_requests[client.fd];
	
	int inPipe[2] = {_requests[client.fd]->inPipe[0], _requests[client.fd]->inPipe[1]};
	int outPipe[2] = {_requests[client.fd]->outPipe[0], _requests[client.fd]->outPipe[1]};
	
	if (cgiRequest.postMethod)
	{
		close(inPipe[READ]);
		close(inPipe[WRITE]);
	} 
	close(outPipe[WRITE]);

	// Dup outpipe for client to read and close dupped fd
	client.resourceReadFd = dup(outPipe[READ]);
	client.cgiStatus = CGI_READ_READY;
	close(outPipe[READ]);
}

void	CGIHandler::handleChildProcess(Client& client)
{
	t_CGIrequest cgiRequest = *_requests[client.fd];
	std::vector<std::string> cgiStrEnv = setCGIEnv(client.requestHandler->getRequest(), client);
	
	int inPipe[2] = {cgiRequest.inPipe[0], cgiRequest.inPipe[1]};
	int outPipe[2] = {cgiRequest.outPipe[0], cgiRequest.outPipe[1]};
	
	// Dup inPipe[READ] to stdin (Client writes request body to other end of pipe) 
	if (dup2(inPipe[READ], STDIN_FILENO) == -1)
	{
		closeFds({clientFd, outPipe[WRITE], outPipe[READ], inPipe[WRITE]});
		std::exit(EXIT_FAILURE);
	}
	close(inPipe[WRITE]);

	// Dup outPipe[WRITE] to STDOUT --> gets set to client read fd
	if (dup2(outPipe[WRITE], STDOUT_FILENO) == -1)
	{
		closeFds({client.fd, outPipe[READ], inPipe[READ], inPipe[WRITE]});
		std::exit(EXIT_FAILURE);
	}
	close(outPipe[READ]);

	struct stat buff;
	if (stat(cgiRequest.CGIPath.c_str(), &buff) != 0)
	{
		perror("Child: File not found");
		closeFds({client.fd, inPipe[READ], outPipe[WRITE]});
		std::exit(EXIT_FAILURE);
	}
	if (!(buff.st_mode & S_IXUSR))
	{
		perror("Child: File is not executable");
		closeFds({client.fd, inPipe[READ], outPipe[WRITE]});
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
	pid_t pid = fork();
	_pids.emplace_back(pid);
	if (pid < 0)
	{
		kill(pid, SIGTERM);
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
	checkProcesses(client.fd, pid);
}

void	CGIHandler::setupCGI(Client& client)
{
	const HttpRequest& request = client.requestHandler->getRequest();
	std::unique_ptr<t_CGIrequest> cgiInst = std::make_unique<t_CGIrequest>();

	cgiInst->CGIPath = setCgiPath(request);
	cgiInst->argv.emplace_back(const_cast<char *>(cgiInst->CGIPath.c_str()));
	cgiInst->argv.emplace_back(nullptr);

	int flags;
	if (request.method == "POST")
	{
		cgiInst->postMethod = true;
		client.resourceOutString = request.body;

		if (pipe(cgiInst->inPipe) < 0)
		{
			throw ServerException(STATUS_INTERNAL_ERROR); // fatal
		}
		if ((flags = fcntl(cgiInst->inPipe[WRITE], F_GETFL)) == -1)
			throw ServerException(STATUS_INTERNAL_ERROR); // fatal
		if (fcntl(cgiInst->inPipe[WRITE], F_SETFL, flags | O_NONBLOCK) == -1) 
			throw ServerException(STATUS_INTERNAL_ERROR); // fatal
			
		client.resourceWriteFd = cgiInst->inPipe[WRITE]; // Set client to write to inpipe
	}
	if (pipe(cgiInst->outPipe) < 0)
	{
		throw ServerException(STATUS_INTERNAL_ERROR); // fatal
	}
	if ((flags = fcntl(cgiInst->outPipe[READ], F_GETFL)) == -1)
		throw ServerException(STATUS_INTERNAL_ERROR); // fatal
	if (fcntl(cgiInst->outPipe[READ], F_SETFL, flags | O_NONBLOCK) == -1) 
		throw ServerException(STATUS_INTERNAL_ERROR); // fatal

	cgiInst->status = CGI_EXECVE_READY;
	_requests.emplace(client.fd, std::move(cgiInst));
}

void	CGIHandler::handleCGI(Client& client)
{
	const HttpRequest& request = client.requestHandler->getRequest();
	if (_requests.size() >= 10)
	{
		std::cout << "Server is busy with too many CGI requests, try again in a moment\n";
		return;
	}
	if (request.method == "GET" || ((request.method == "POST") && !readyForExecve(client)))
	{
		setupCGI(client);
	}
	if (readyForExecve(client))
	{
		runCGIScript(client);
	}
}
