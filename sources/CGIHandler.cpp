#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <exception>
#include <iostream>
#include <filesystem>
#include <dirent.h>
#include "CGIHandler.hpp"
#include "HttpRequest.hpp"
#include "ServerException.hpp"

CGIHandler::CGIHandler()
{
	_requests.reserve(10);
}

CGIHandler::~CGIHandler() {}

void	CGIHandler::closeAllOpenFds()
{
	DIR *dir = opendir("/proc/self/fd");
	if (dir == nullptr)
		return;
	int dir_fd = dirfd(dir);
	struct dirent *entry;

	while ((entry = readdir(dir)))
	{
		int fd = atoi(entry->d_name);
		if (fd > 2 && fd != dir_fd && fd < 1024)
			close(fd);
	}
	closedir(dir);
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

void	CGIHandler::cleanupPid(pid_t pid)
{
	int i = 0;

	for (pid_t it : _pids)
	{
		if (it == pid)
		{
			std::cout << "Pid " << pid << " erased\n";
			_pids.erase(_pids.begin() + i);
		}
		i++;
	}
}

void	CGIHandler::killCGIProcess(Client& client)
{
	if (client.cgiStatus != CGI_RESPONSE_READY &&
		(!_requests.empty() && _requests.find(client.fd) != _requests.end()))
	{
		if (client.cgiStatus == CGI_FORKED)
		{
			if (checkProcess(client.fd) == CGI_FORKED)
			{
				kill(_requests[client.fd]->childPid, SIGTERM);
			}
		}
		cleanupPid(_requests[client.fd]->childPid);
		_requests.erase(client.fd);
	}
}

int	CGIHandler::cleanupCGI(Client& client)
{
	// CGI request is completed and the response is read by the client
	_requests.erase(client.fd);
	return CGI_RESPONSE_READY;
}

/*	Checks if child has been terminated and output written to client, 
	CGI request completed cleanup resources and return read ready status to client*/
int	CGIHandler::checkProcess(int clientFd)
{
	int status;
	pid_t pid = _requests[clientFd]->childPid;
	pid_t w_res = waitpid(pid, &status, WNOHANG);
	
	if (w_res == 0)
		return CGI_FORKED;
	else if (w_res == -1)
	{
		std::cerr << "Waitpid error " << strerror(errno) << "\n";
		throw ServerException(STATUS_INTERNAL_ERROR);
	}
	else
	{
		if (WIFEXITED(status))
		{
			std::cout << "Child exited with satus " << WEXITSTATUS(status) << "\n";
			cleanupPid(_requests[clientFd]->childPid);
			return CGI_READ_READY;
		}
		else if (WIFSIGNALED(status))
		{
			std::cerr << "Child killed by signal" << strerror(errno) << "\n";
			return CGI_CHILD_KILLED;
			//throw ServerException(STATUS_INTERNAL_ERROR);
		}
	}
	return CGI_ERROR;
}

void	CGIHandler::validateCGIScript(t_CGIrequest cgiRequest)
{
	struct stat buff;
	if (stat(cgiRequest.CGIPath.c_str(), &buff) != 0)
	{
		std::cerr << "Child: File not found\n";
		closeAllOpenFds();
		std::exit(EXIT_FAILURE);
	}
	if (!(buff.st_mode & S_IXUSR))
	{
		std::cerr << "Child: File is not executable\n";
		closeAllOpenFds();
		std::exit(EXIT_FAILURE);
	}
}

void	CGIHandler::setPipesToNonBlock(int* pipe)
{
	int flags;
	
	// Write end
	if ((flags = fcntl(pipe[WRITE], F_GETFL)) == -1)
		throw ServerException(STATUS_INTERNAL_ERROR);
	if (fcntl(pipe[WRITE], F_SETFL, flags | O_NONBLOCK) == -1) 
		throw ServerException(STATUS_INTERNAL_ERROR);
	
	// Read end
	if ((flags = fcntl(pipe[READ], F_GETFL)) == -1)
		throw ServerException(STATUS_INTERNAL_ERROR);
	if (fcntl(pipe[READ], F_SETFL, flags | O_NONBLOCK) == -1) 
		throw ServerException(STATUS_INTERNAL_ERROR);
}

bool	CGIHandler::readyForExecve(const Client& client)
{
	if (!_requests.empty() && _requests.find(client.fd) != _requests.end())
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
	strEnv.emplace_back("REMOTE_ADDR=");	// not necessarily needed

	return strEnv;
}

void CGIHandler::handleParentProcess(Client& client, pid_t pid)
{
	t_CGIrequest& cgiRequest = *_requests[client.fd];
	
	_pids.emplace_back(pid);
	std::cout << "Process has forked " << pid << " added\n";
	client.cgiStatus = CGI_FORKED;
	cgiRequest.childPid = pid;
	
	int inPipe[2] = {_requests[client.fd]->inPipe[0], _requests[client.fd]->inPipe[1]};
	int outPipe[2] = {_requests[client.fd]->outPipe[0], _requests[client.fd]->outPipe[1]};
	
	if (client.requestHandler->getMethod() == "POST")
		close(inPipe[READ]);
	close(outPipe[WRITE]);

	client.resourceReadFd = dup(outPipe[READ]);
	if (client.resourceReadFd == -1)
	{
		std::cerr << "Dup error:" << strerror(errno) << "\n";
		throw ServerException(STATUS_INTERNAL_ERROR);
	}
	close(outPipe[READ]);
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

	validateCGIScript(cgiRequest);
	std::vector<char*> envp;
	for (const auto &var : cgiStrEnv)
	{
		envp.emplace_back(const_cast<char *>(var.c_str()));
	}
	envp.emplace_back(nullptr);

	closeAllOpenFds();
	execve(cgiRequest.CGIPath.c_str(), cgiRequest.argv.data(), envp.data());
	std::cerr << "Child: Execve failed" << strerror(errno) << "\n";
	std::exit(EXIT_FAILURE);
}

void	CGIHandler::setupCGI(Client& client)
{
	const HttpRequest& request = client.requestHandler->getRequest();
	std::unique_ptr<t_CGIrequest> cgiInst = std::make_unique<t_CGIrequest>();

	cgiInst->CGIPath = setCgiPath(request);
	cgiInst->argv.emplace_back(const_cast<char *>(cgiInst->CGIPath.c_str()));
	cgiInst->argv.emplace_back(nullptr);

	if (request.method == "POST")
	{
		client.resourceOutString = request.body;

		if (pipe(cgiInst->inPipe) < 0)
		{
			std::cerr << "Pipe error: " << strerror(errno) << "\n";
			throw ServerException(STATUS_INTERNAL_ERROR);
		}
		setPipesToNonBlock(cgiInst->inPipe);
			
		// Set client to write to inpipe and close unused pipe
		client.resourceWriteFd = dup(cgiInst->inPipe[WRITE]);
		if (client.resourceWriteFd == -1)
		{
			std::cerr << "Dup error: " << strerror(errno) << "\n";
			throw ServerException(STATUS_INTERNAL_ERROR);
		}
		close(cgiInst->inPipe[WRITE]);
	}

	if (pipe(cgiInst->outPipe) < 0)
	{
		std::cerr << "Pipe error: " << strerror(errno) << "\n";
		throw ServerException(STATUS_INTERNAL_ERROR);
	}
	setPipesToNonBlock(cgiInst->inPipe);

	_requests.emplace(client.fd, std::move(cgiInst));
	client.cgiStatus = CGI_EXECVE_READY;
}


void	CGIHandler::runCGIScript(Client& client)
{
	if (client.cgiStatus != CGI_FORKED)
	{
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
	if (_requests.size() >= 10)
	{
		std::cout << "Server is busy with too many CGI requests, try again in a moment\n";
		throw ServerException(STATUS_NOT_ALLOWED); // handle sending some error for overloaded CGI
	}
	if (request.method == "GET" || ((request.method == "POST") && !readyForExecve(client)))
	{
		setupCGI(client); // sets CGI status to EXECVE READY
	}
	if (readyForExecve(client))
	{
		runCGIScript(client); // sets CGI status FORKED
	}
	if (client.cgiStatus == CGI_FORKED)
		client.cgiStatus = checkProcess(client.fd); // sets FORKED (still running) or READ_READY
}
