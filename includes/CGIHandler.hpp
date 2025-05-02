#pragma once

#include <vector>
#include "Client.hpp"

#define READ 0
#define WRITE 1

enum CGIStatus {
	CGI_FORKED,
	CGI_ERROR,
	CGI_EXECVE_READY,
	CGI_READ_READY,
	CGI_RESPONSE_READY,
	CGI_CHILD_KILLED,
	CGI_CHILD_STOPPED
};

typedef struct s_CGIrequest {
	int					inPipe[2];
	int					outPipe[2];
	pid_t				childPid;
	std::string			output;
	std::vector<char*>	argv;
	std::vector<char*>	envp;
	std::string			CGIPath;
	int					childExitStatus;
} t_CGIrequest;

class CGIHandler {
private:
	static std::unordered_map<int, std::unique_ptr<t_CGIrequest>>	_requests;
	static std::vector<pid_t> 										_pids;

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
	static void					cleanupPid(pid_t pid, int exitStatus);

	public:
	CGIHandler();
	~CGIHandler();
	
	void	handleCGI(Client& client);
	void	cleanupCGI(Client& client);
	void	killCGIProcess(Client& client);
	
	// Signal handler
	static void					checkProcesses(int sig);
};
