#include <sys/wait.h>
#include <sys/stat.h>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <exception>
#include <iostream>
#include <filesystem>
#include <dirent.h>
#include "HttpRequest.hpp"
#include "CGIHandler.hpp"
#include "ServerException.hpp"
#include "Logger.hpp"

CGIHandler::CGIHandler()
{
	_requests.reserve(CGI_MAX_REQUESTS);
}

CGIHandler::~CGIHandler() {}

void	CGIHandler::closeAllOpenFds()
{
	DIR *dir = opendir("/proc/self/fd");
	if (dir == nullptr)
		return;
	int dir_fd = dirfd(dir);
	struct dirent *entry;

	while ((entry = readdir(dir)) != nullptr)
	{
		int fd = atoi(entry->d_name);
		if (fd > 2 && fd != dir_fd && fd < 1024)
			close(fd);
	}
	closedir(dir);
}

std::string CGIHandler::setCgiPath(Client& client)
{
	const HttpRequest& request = client.requestHandler->getRequest();

	// Get root of cgi-bin location + build executable path
	std::string CGIroot;
	std::string CGIpath;
	const Location* location = ServerConfigData::getParentLocation(*client.serverConfig, request.uriPath);

	CGIroot = location->root;
	CGIpath = location->cgiPath;
		//if (!CGIroot.empty() && parsedUri.find("/cgi-bin") == 0) 
		//	parsedUri = parsedUri.substr(std::string("/cgi-bin").length());
	std::string cgiUri = std::filesystem::current_path().string() + "/" + CGIroot + "/" + CGIpath;
	
	validateCGIScript(cgiUri);
	return cgiUri;
}

void	CGIHandler::cleanupPid(pid_t pid)
{
	int i = 0;

	for (pid_t it : _pids)
	{
		if (it == pid)
			_pids.erase(_pids.begin() + i);
		i++;
	}
}

void	CGIHandler::killCGIProcess(Client& client)
{
	if (client.cgiStatus == CGI_FORKED && (_requests.find(client.fd) != _requests.end()))
	{
		kill(_requests[client.fd]->childPid, SIGTERM);
		waitpid(_requests[client.fd]->childPid, nullptr, 0);
		cleanupPid(_requests[client.fd]->childPid);
	}
}

void	CGIHandler::cleanupCGI(Client& client)
{
	// CGI request is completed and the response is read by the client
	if (_requests.find(client.fd) == _requests.end())
		return;

	if (client.cgiStatus == CGI_FORKED)
		killCGIProcess(client);

	if (_requests[client.fd]->inPipe[READ] != -1)
		close(_requests[client.fd]->inPipe[READ]);
	if (_requests[client.fd]->inPipe[WRITE] != -1)
		close(_requests[client.fd]->inPipe[WRITE]);
	if (_requests[client.fd]->outPipe[READ] != -1)
		close(_requests[client.fd]->outPipe[READ]);
	if (_requests[client.fd]->outPipe[WRITE] != -1)
		close(_requests[client.fd]->outPipe[WRITE]);
	_requests.erase(client.fd);
}

void	CGIHandler::checkCGIStatus(Client& client)
{
	if (client.cgiStatus == CGI_BAD_GATEWAY)
		throw ServerException(STATUS_BAD_GATEWAY);
	if (client.cgiStatus == CGI_TIMED_OUT)
		throw ServerException(STATUS_GATEWAY_TIMEOOUT);
	if (client.cgiStatus == CGI_SERVER_ERROR)
		throw ServerException(STATUS_INTERNAL_ERROR);
}

bool	CGIHandler::cgiTimeout(Client& client)
{
	const std::time_t now = std::time(nullptr);
	if (client.cgiStatus == CGI_FORKED && (now - _requests[client.fd]->CGIstart > CGI_TIMEOUT))
	{
		Logger::log(Logger::OK,  "Client " + std::to_string(client.fd) + " CGI timed out, killing process with id " + std::to_string(_requests[client.fd]->childPid));
		return false;
	}
	return true;
}

/*	Checks if child has been terminated and output written to client, 
	CGI request completed cleanup resources and return read ready status to client*/
void	CGIHandler::checkProcess(Client& client)
{
	int status;
	pid_t pid;
	
	if (_requests.find(client.fd) == _requests.end() || client.cgiStatus == CGI_COMPLETE)
		return;
	if ((pid = waitpid(_requests[client.fd]->childPid, &status, WNOHANG)) > 0)
	{
		if (WIFEXITED(status))
		{
			if (WEXITSTATUS(status) > 0)
				client.cgiStatus = CGI_BAD_GATEWAY;
			else
				client.cgiStatus = CGI_CHILD_EXITED;
		}
		else if (WIFSIGNALED(status))
		{
			Logger::log(Logger::ERROR, "Child killed by signal: " + std::to_string(WTERMSIG(status)));
			client.cgiStatus = CGI_CHILD_KILLED;
		}
		else if (WIFSTOPPED(status))
		{
			Logger::log(Logger::ERROR, "Child stopped by signal: " + std::to_string(WSTOPSIG(status)));
			client.cgiStatus = CGI_CHILD_STOPPED;
		}
		cleanupPid(pid);
	}
	if (pid == 0)
	{
		if (!cgiTimeout(client))
		{
			killCGIProcess(client);
			client.cgiStatus = CGI_TIMED_OUT;
		}
	}
	if (pid == -1 && errno != ECHILD)
	{
		Logger::log(Logger::ERROR, "Waitpid error" + std::string(std::strerror(errno)));
		client.cgiStatus = CGI_SERVER_ERROR;
	}
	if (!cgiTimeout(client) && client.cgiStatus != CGI_EXECVE_READY)
	{
		killCGIProcess(client);
		client.cgiStatus = CGI_TIMED_OUT;
	}
}

void	CGIHandler::validateCGIScript(std::string CGIExecutablePath)
{
	struct stat buff;
	if (stat(CGIExecutablePath.c_str(), &buff) != 0)
		throw ServerException(STATUS_NOT_FOUND);
	if (!(buff.st_mode & S_IXUSR))
		throw ServerException(STATUS_FORBIDDEN);
}

void	CGIHandler::setPipesToNonBlock(int* pipe)
{
	int flags;
	
	if ((flags = fcntl(pipe[WRITE], F_GETFL)) == -1)
		throw ServerException(STATUS_INTERNAL_ERROR);
	if (fcntl(pipe[WRITE], F_SETFL, flags | O_NONBLOCK) == -1) 
		throw ServerException(STATUS_INTERNAL_ERROR);
	
	if ((flags = fcntl(pipe[READ], F_GETFL)) == -1)
		throw ServerException(STATUS_INTERNAL_ERROR);
	if (fcntl(pipe[READ], F_SETFL, flags | O_NONBLOCK) == -1) 
		throw ServerException(STATUS_INTERNAL_ERROR);
}

bool	CGIHandler::readyForExecve(const Client& client)
{
	if (_requests.find(client.fd) != _requests.end())
	{
		if (client.cgiStatus == CGI_EXECVE_READY && client.resourceWriteFd == -1)
			return true;
		else if (client.cgiStatus == CGI_EXECVE_READY && client.requestHandler->getMethod() != "POST")
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
		auto contenType = request.headers.find("Content-Type");
		if (contenType != request.headers.end())
			strEnv.emplace_back("CONTENT_TYPE=" + request.headers.at("Content-Type"));
		auto contentLength = request.headers.find("Content-Length");
		if (contentLength != request.headers.end())
			strEnv.emplace_back("CONTENT_LENGTH=" + request.headers.at("Content-Length"));
	}
	strEnv.emplace_back("QUERY_STRING=" + request.uriQuery);
	strEnv.emplace_back("PATH_INFO=" + request.uri);
	strEnv.emplace_back("SERVER_PORT=" + client.serverConfig->port);

	return strEnv;
}

void CGIHandler::handleParentProcess(Client& client, pid_t pid)
{
	t_CGIrequest& cgiRequest = *_requests[client.fd];
	
	_pids.emplace_back(pid);
	Logger::log(Logger::OK, "Client " + std::to_string(client.fd) + " forked: process " + std::to_string(pid) + " added");

	client.cgiStatus = CGI_FORKED;
	cgiRequest.childPid = pid;
	
	int inPipe[2] = {_requests[client.fd]->inPipe[READ], _requests[client.fd]->inPipe[WRITE]};
	int outPipe[2] = {_requests[client.fd]->outPipe[READ], _requests[client.fd]->outPipe[WRITE]};
	
	if (client.requestHandler->getMethod() == "POST")
	{
		close(inPipe[READ]);
		_requests[client.fd]->inPipe[READ] = -1;
	}
	close(outPipe[WRITE]);
	_requests[client.fd]->outPipe[WRITE] = -1;

	client.resourceReadFd = dup(outPipe[READ]);
	close(outPipe[READ]);
	_requests[client.fd]->outPipe[READ] = -1;
	if (client.resourceReadFd == -1)
	{
		Logger::log(Logger::ERROR, "Dup error:" + std::string(std::strerror(errno)));		
		throw ServerException(STATUS_INTERNAL_ERROR);
	}
}

void	CGIHandler::handleChildProcess(Client& client)
{
	t_CGIrequest cgiRequest = *_requests[client.fd];
	std::vector<std::string> cgiStrEnv = setCGIEnv(client.requestHandler->getRequest(), client);
	int inPipe[2] = {cgiRequest.inPipe[0], cgiRequest.inPipe[1]};
	int outPipe[2] = {cgiRequest.outPipe[0], cgiRequest.outPipe[1]};
	
	if (client.requestHandler->getMethod() == "POST")
	{
		if (dup2(inPipe[READ], STDIN_FILENO) == -1)
		{
			closeAllOpenFds();
			std::exit(EXIT_FAILURE);
		}
	}
	if (dup2(outPipe[WRITE], STDOUT_FILENO) == -1)
	{
		closeAllOpenFds();
		std::exit(EXIT_FAILURE);
	}
	close(outPipe[READ]);
	close(outPipe[WRITE]);

	std::vector<char*> envp;
	for (const auto &var : cgiStrEnv)
	{
		envp.emplace_back(const_cast<char *>(var.c_str()));
	}
	envp.emplace_back(nullptr);

	closeAllOpenFds();
	execve(cgiRequest.CGIPath.c_str(), cgiRequest.argv.data(), envp.data());
	Logger::log(Logger::ERROR, "Execve failed: " + std::string(std::strerror(errno)));
	std::exit(EXIT_FAILURE);
}

void	CGIHandler::setupCGI(Client& client)
{
	const HttpRequest& request = client.requestHandler->getRequest();
	std::unique_ptr<t_CGIrequest> cgiInst = std::make_unique<t_CGIrequest>();

	cgiInst->CGIPath = setCgiPath(client);

	cgiInst->inPipe[READ] = -1;
	cgiInst->inPipe[WRITE] = -1;
	cgiInst->outPipe[READ] = -1;
	cgiInst->outPipe[WRITE] = -1;

	cgiInst->argv.emplace_back(const_cast<char *>(cgiInst->CGIPath.c_str()));
	cgiInst->argv.emplace_back(nullptr);

	if (request.method == "POST")
	{
		client.resourceOutString = request.body;

		if (pipe(cgiInst->inPipe) < 0)
		{
			Logger::log(Logger::ERROR, "Pipe error: " + std::string(std::strerror(errno)));
			throw ServerException(STATUS_INTERNAL_ERROR);
		}
		setPipesToNonBlock(cgiInst->inPipe);
			
		// Set client to write to inpipe and close unused pipe
		client.resourceWriteFd = dup(cgiInst->inPipe[WRITE]);
		if (client.resourceWriteFd == -1)
		{
			Logger::log(Logger::ERROR, "Dup error: " + std::string(std::strerror(errno)));
			throw ServerException(STATUS_INTERNAL_ERROR);
		}
		close(cgiInst->inPipe[WRITE]);
		cgiInst->inPipe[WRITE] = -1;
	}
	if (pipe(cgiInst->outPipe) < 0)
	{
		Logger::log(Logger::ERROR, "Pipe error: " + std::string(std::strerror(errno)));
		throw ServerException(STATUS_INTERNAL_ERROR);
	}
	setPipesToNonBlock(cgiInst->outPipe);

	_requests.emplace(client.fd, std::move(cgiInst));
	client.cgiStatus = CGI_EXECVE_READY;
}


void	CGIHandler::runCGIScript(Client& client)
{
	if (client.cgiStatus != CGI_FORKED)
	{
		_requests[client.fd]->CGIstart = std::time(nullptr);
		
		pid_t pid = fork();
		if (pid < 0)
		{
			throw ServerException(STATUS_INTERNAL_ERROR);
		}
		if (pid == 0)
		{
			handleChildProcess(client);
		}
		else
		{
			handleParentProcess(client, pid);
		}
	}
}

void	CGIHandler::handleCGI(Client& client)
{
	const HttpRequest& request = client.requestHandler->getRequest();

	checkCGIStatus(client);
	if ((client.cgiStatus == CGI_COMPLETE) || (client.cgiStatus == CGI_FORKED))
		return;
 	if (client.cgiStatus == CGI_CHILD_EXITED)
	{
		client.cgiStatus = CGI_COMPLETE;
		cleanupCGI(client);
		return;
	}
	if (_requests.size() >= CGI_MAX_REQUESTS && !readyForExecve(client)) // not for forked ones
	{
		Logger::log(Logger::ERROR, "Client " + std::to_string(client.fd) + "Server is busy with too many CGI requests, try again in a moment");
		throw ServerException(STATUS_SERVICE_UNAVAILABLE);
	}
	if (request.method == "GET" || ((request.method == "POST") && !readyForExecve(client)))
	{
		Logger::log(Logger::OK, "Client " + std::to_string(client.fd) + " CGI request initialised");
		setupCGI(client); // sets CGI status to EXECVE READY
	}
	if (readyForExecve(client))
	{
		runCGIScript(client); // sets CGI status FORKED
	}
}