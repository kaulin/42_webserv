#pragma once

#include "webserv.hpp"
#include "Request.hpp"
#include "ServerHandler.hpp"

#define READ 0
#define WRITE 1

class 	Request;
struct	s_client;

enum CGIStatus {
	READY,
	CLOSED,
	ERROR
};

typedef struct s_CGIrequest {
	int 						status;
	int							pipe[2];
	int							childPid;
	//std::vector<char* const>	argv; // args for execve call
	std::vector<char*>			envp;
	std::string					output;
	std::vector<std::string>	CGIEnv;
} t_CGIrequest;

class CGIHandler {
private:
	std::unordered_map<int, s_CGIrequest>	_requests; // limit to 10

	// Private class methods
	void	handleChildProcess(s_CGIrequest request);
	void	handleParentProcess(s_CGIrequest request);
	void	setCGIEnv(const HttpRequest &request);
public:
	CGIHandler();

	void			setupCGI(s_client client);
	void			runCGIScript(s_client client);
};
