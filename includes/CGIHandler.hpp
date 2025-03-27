#pragma once

#include <vector>
#include "Request.hpp"

#define READ 0
#define WRITE 1

struct	s_client;

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
	std::vector<std::string>	CGIEnv;
	std::string					CGIPath;
} t_CGIrequest;

class CGIHandler {
private:
	std::unordered_map<int, t_CGIrequest>	_requests; // limit to 10

	// Private class methods
	void 						closeFds(const std::vector<int> fdsToclose);
	void 						setCGIEnv(t_CGIrequest &cgiRequest, std::vector<char *> &envp);
	void 						handleChildProcess(t_CGIrequest request, s_client& client);
	void						handleParentProcess(t_CGIrequest request);
	std::vector<std::string>	initCGIEnv(HttpRequest& request);
public:
	CGIHandler();

	void			setupCGI(s_client& client);
	void			runCGIScript(s_client& client);
};
