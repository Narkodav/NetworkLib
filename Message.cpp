#include "Message.h"

namespace http
{
	std::string Request::getFirstLine() const
	{
		return methodToString(method) + " " + uri + " " + version + "\r\n";
	}

	std::string Response::getFirstLine() const
	{
		if (statusMessage != "")
			return version + " " +
			std::to_string(static_cast<int>(statusCode)) + " " +
			statusMessage + "\r\n";
		else
			return version + " " + std::to_string(static_cast<int>(statusCode)) + " " +
			statusCodeToString(statusCode) + "\r\n";
	}



}