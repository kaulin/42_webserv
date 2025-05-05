#include <sys/wait.h>
#include <sys/stat.h>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <exception>
#include <iostream>
#include <filesystem>
#include <dirent.h>
#include "CGIHandler.hpp"
#include "HttpRequest.hpp"
#include "ServerException.hpp"
// #include "ServerHandler.hpp"
#include "Logger.hpp"

std::unordered_map<int, std::unique_ptr<t_CGIrequest>>	CGIHandler::_requests;
std::vector<pid_t> 										CGIHandler::_pids;

extern sig_atomic_t g_cgiCheckProcess;

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

	std::string parsedUri = request.uri;
	if (!request.uriQuery.empty() && parsedUri.find('?') != std::string::npos)
		parsedUri = request.uri.substr(0, request.uri.find('?'));

	// Get root of cgi-bin location + build executable path
	std::string CGIroot;
	auto CGILlocationSettings = client.serverConfig->locations.find("/cgi-bin");
	if (CGILlocationSettings != client.serverConfig->locations.end())
	{
		CGIroot = CGILlocationSettings->second.root;
		if (!CGIroot.empty() && parsedUri.find("/cgi-bin") == 0) 
			parsedUri = parsedUri.substr(std::string("/cgi-bin").length());
	}
	std::string cgiUri = std::filesystem::current_path().string() + "/" + CGIroot + parsedUri;
	
	validateCGIScript(cgiUri); // throws error if not found
	return cgiUri;
}

/* void	CGIHandler::cleanupPid(pid_t pid, int exitStatus)
{
	int i = 0;

	for (auto& request : _requests)
	{
		if (request.second->childPid == pid)
			request.second->childExitStatus = exitStatus;
	}
	for (pid_t it : _pids)
	{
		if (it == pid)
		{
			Logger::log(Logger::OK, "Removing pid " + std::to_string(pid));
			_pids.erase(_pids.begin() + i);
		}
		i++;
	}
	Logger::log(Logger::OK, "leaving cleanup pid");

} */


void	CGIHandler::cleanupPid(pid_t pid)
{
	int i = 0;

	for (pid_t it : _pids)
	{
		if (it == pid)
		{
			Logger::log(Logger::OK, "Removing pid " + std::to_string(pid));
			_pids.erase(_pids.begin() + i);
		}
		i++;
	}
}

void	CGIHandler::killCGIProcess(Client& client)
{
	_requests.erase(client.fd);
	if (client.cgiStatus == CGI_ERROR)
	{
		//close(client.resourceReadFd);
		//cleanupPid(_requests[client.fd]->childPid); // check
	}
	// IF TIMEOUT --> kill and wait etc
	if (client.cgiStatus != CGI_RESPONSE_READY &&
		(!_requests.empty() && _requests.find(client.fd) != _requests.end()))
	{
		/* if (client.cgiStatus == CGI_FORKED)
		{
			kill(_requests[client.fd]->childPid, SIGTERM);
		} */
		//cleanupPid(_requests[client.fd]->childPid); // check
		_requests.erase(client.fd);
	}
}

void	CGIHandler::cleanupCGI(Client& client)
{
	// CGI request is completed and the response is read by the client
	Logger::log(Logger::OK, "Cleaning up CGI for client " + std::to_string(client.fd));

	//cleanupPid(_requests[client.fd]->childPid);
	_requests.erase(client.fd);
}

/*	Checks if child has been terminated and output written to client, 
	CGI request completed cleanup resources and return read ready status to client*/
void	CGIHandler::checkProcess(Client& client)
{
	int status;
	pid_t pid;
	
	std::cout << "Entering waitpid loop\n";
	if (_requests.find(client.fd) == _requests.end())
	{
		std::cout << "No pid for client anymore\n";
		return;
	}
	while((pid = waitpid(_requests[client.fd]->childPid, &status, WNOHANG)) > 0)
	{
		std::cout << "Waitpid loop\n";
		if (WIFEXITED(status))
		{
			int exitCode = WEXITSTATUS(status);
			Logger::log(Logger::OK, "Child exited with status: " + std::to_string(exitCode));
			if (exitCode > 0)
			{
				client.cgiStatus = CGI_ERROR;
				cleanupPid(pid);
			}
			else
			{
				client.cgiStatus = CGI_CHILD_EXITED;
				cleanupPid(pid);
			}
		}
		else if (WIFSIGNALED(status))
		{
			Logger::log(Logger::ERROR, "Child killed by signal: " + std::to_string(WTERMSIG(status)));
			client.cgiStatus = CGI_CHILD_KILLED;
			cleanupPid(pid);
		}
		else if (WIFSTOPPED(status))
		{
			Logger::log(Logger::ERROR, "Child stopped by signal: " + std::to_string(WSTOPSIG(status)));
			client.cgiStatus = CGI_CHILD_STOPPED;
			cleanupPid(pid);
		}
	}
	if (pid == 0)
	{
		std::cout << "Client " << client.fd << " is still running\n";
	}
	if (pid == -1 && errno != ECHILD)
	{
		Logger::log(Logger::ERROR, "CGI error (waitpid): " + std::string(std::strerror(errno)));
		throw ServerException(STATUS_INTERNAL_ERROR);
	}
	Logger::log(Logger::OK, "Finished checking child process with status " + std::to_string(client.cgiStatus));
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

	return strEnv;
}

void	CGIHandler::childTimeout(int signal)
{
	(void)signal;
	int i = 0;

	std::cout << "Child has timed out\n";
	for (pid_t it : _pids)
	{
		if (it > 0)
		{
			std::cout << "Pid killed and removed" << it << "\n";
			if (kill(it, SIGTERM) == -1)
				std::cout << strerror(errno) << "\n";
			int status;
			waitpid(it, &status, 0);

			_pids.erase(_pids.begin() + i);
		}
		i++;
	}
}

void CGIHandler::handleParentProcess(Client& client, pid_t pid)
{
	t_CGIrequest& cgiRequest = *_requests[client.fd];
	
	alarm(0);
	_pids.emplace_back(pid);
	Logger::log(Logger::OK, "Process has forked " + std::to_string(pid) + " added");
	
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
		Logger::log(Logger::ERROR, "Dup error:" + std::string(std::strerror(errno)));		
		throw ServerException(STATUS_INTERNAL_ERROR);
	}
	close(outPipe[READ]);
}

/* void	CGIHandler::flagKill(int signal)
{

} */

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
	//signal(SIGALRM, flagKill);
	execve(cgiRequest.CGIPath.c_str(), cgiRequest.argv.data(), envp.data());
	Logger::log(Logger::ERROR, "Execve failed: " + std::string(std::strerror(errno)));
	std::exit(EXIT_FAILURE);
}

void	CGIHandler::setupCGI(Client& client)
{
	const HttpRequest& request = client.requestHandler->getRequest();
	std::unique_ptr<t_CGIrequest> cgiInst = std::make_unique<t_CGIrequest>();

	cgiInst->CGIPath = setCgiPath(client);

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
	}

	if (pipe(cgiInst->outPipe) < 0)
	{
		Logger::log(Logger::ERROR, "Pipe error: " + std::string(std::strerror(errno)));
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
		// signal(SIGALRM, childTimeout);
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
	Logger::log(Logger::OK, "Handling CGI for request " + std::to_string(client.fd));
	const HttpRequest& request = client.requestHandler->getRequest();

 	if (client.cgiStatus == CGI_CHILD_EXITED)
	{
		cleanupCGI(client);
		client.cgiStatus = CGI_RESPONSE_READY;
		return;
	}
	if (!_requests.empty() && (_requests.find(client.fd) != _requests.end()))
	{
		if (_requests[client.fd]->childExitStatus == CGI_ERROR)
		{
			client.cgiStatus = CGI_ERROR;
			throw ServerException(STATUS_INTERNAL_ERROR);
		}
	}
	if (_requests.size() >= 10)
	{
		Logger::log(Logger::ERROR, "Server is busy with too many CGI requests, try again in a moment");
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
	// IF signal
	/* if (client.cgiStatus == CGI_FORKED)
		checkProcess(client); */
	/* if (client.cgiStatus == CGI_ERROR)
		throw ServerException(STATUS_INTERNAL_ERROR); */
}
