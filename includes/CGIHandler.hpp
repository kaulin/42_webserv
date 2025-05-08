#pragma once

#include <vector>
#include "Client.hpp"

#define READ 0
#define WRITE 1

#define CGI_TIMEOUT 3
#define CGI_MAX_REQUESTS 10

enum CGIStatus {
	CGI_FORKED,
	CGI_SERVER_ERROR,
	CGI_BAD_GATEWAY,
	CGI_EXECVE_READY,
	CGI_READ_READY,
	CGI_RESPONSE_READY,
	CGI_COMPLETE,
	CGI_CHILD_KILLED,
	CGI_CHILD_STOPPED,
	CGI_TIMED_OUT,
	CGI_CHILD_EXITED
};

typedef struct s_CGIrequest {
	int					inPipe[2];
	int					outPipe[2];
	pid_t				childPid;
	std::time_t			CGIstart;
	std::string			output;
	std::vector<char*>	argv;
	std::vector<char*>	envp;
	std::string			CGIPath;
} t_CGIrequest;

class CGIHandler {
private:
	std::unordered_map<int, std::unique_ptr<t_CGIrequest>>	_requests;
	std::vector<pid_t> 										_pids;

	void 						handleChildProcess(Client& client);
	void						handleParentProcess(Client& client, pid_t pid);
	std::string					setCgiPath(Client& client);
	std::vector<std::string>	setCGIEnv(const HttpRequest& request, const Client& client);
	void						validateCGIScript(std::string CGIExecutablePath);
	void						setupCGI(Client& client);
	void						runCGIScript(Client& client);
	bool						readyForExecve(const Client& client);
	void						setPipesToNonBlock(int* pipe);
	void						closeAllOpenFds();
	void						cleanupPid(pid_t pid);
	bool						cgiTimeout(Client& client);

	public:
	CGIHandler();
	~CGIHandler();
	
	void	checkProcess(Client& client);
	void	handleCGI(Client& client);
	void	cleanupCGI(Client& client);
	void	killCGIProcess(Client& client);
	void	checkCGIStatus(Client& client);
};
