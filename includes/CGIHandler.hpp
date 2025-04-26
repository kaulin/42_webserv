#pragma once

#include <vector>
#include "Client.hpp"

#define READ 0
#define WRITE 1

enum CGIStatus {
	CGI_FORKED,
	CGI_EXECVE_READY,
	CGI_READ_READY,
	CGI_RESPONSE_READY,
	CGI_CHILD_KILLED,
	CGI_ERROR
};

typedef struct s_CGIrequest {
	int					inPipe[2];
	int					outPipe[2];
	pid_t				childPid;
	std::string			output;
	std::vector<char*>	argv;
	std::vector<char*>	envp;
	std::string			CGIPath;
} t_CGIrequest;

class CGIHandler {
private:
	std::unordered_map<int, std::unique_ptr<t_CGIrequest>>	_requests;
	std::vector<pid_t> 										_pids;

	// Private class methods
	void 						closeFds(const std::vector<int> fdsToclose);
	void 						handleChildProcess(Client& client);
	void						handleParentProcess(Client& client, pid_t pid);
	std::string					setCgiPath(const HttpRequest& request);
	std::vector<std::string>	setCGIEnv(const HttpRequest& request, const Client& client);
	void						validateCGIScript(t_CGIrequest cgiRequest, Client& client);
	void						setupCGI(Client& client);
	void						runCGIScript(Client& client);
	bool						readyForExecve(const Client& client);
	int							checkProcess(int clientFd);
	void						cleanupPid(pid_t pid);
	void						setPipesToNonBlock(int* pipe);
public:
	CGIHandler();
	~CGIHandler();

	void	handleCGI(Client& client);
	int		cleanupCGI(Client& client);
	void	killCGIProcess(Client& client);
};
