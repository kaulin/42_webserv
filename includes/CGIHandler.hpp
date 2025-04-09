#pragma once

#include <vector>
#include "Client.hpp"

#define READ 0
#define WRITE 1

enum CGIStatus {
	CGI_UNSET,
	CGI_READY,
	CGI_FORKED,
	CGI_COMPLETE,
	CGI_ERROR
};

typedef struct s_CGIrequest {
	int 						status;
	int							pipe[2];
	pid_t						childPid;
	std::string					output;
	std::vector<char*>			argv;
	std::vector<char*>			envp;
	std::string					CGIPath;
} t_CGIrequest;

class CGIHandler {
private:
	std::unordered_map<int, std::unique_ptr<t_CGIrequest>> _requests;

	// Private class methods
	void 				closeFds(const std::vector<int> fdsToclose);
	void 				handleChildProcess(int clientFd);
	std::vector<char*>	setCGIEnv(const HttpRequest& request, const Client& client);
public:
	CGIHandler();

	void			setupCGI(Client& client);
	void			runCGIScript(Client& client);
};
