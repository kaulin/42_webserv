#pragma once

#include <string>
#include <netdb.h>

class DNS
{
	public:
		static std::string	convertBinaryToIPv4(uint32_t ip_binary);
		static bool			resolveDNS(const std::string& hostname, std::string& ip_address);

};