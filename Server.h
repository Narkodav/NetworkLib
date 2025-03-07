#pragma once
#include "Acceptor.h"
#include "IOContext.h"
#include "Parser.h"

namespace http
{
	class Server
	{
	private:
		IOContext& m_context;
		Acceptor m_acceptor;


	public:
		
		Server(IOContext& context, int port) :
			m_context(context), m_acceptor(context, port) {};

		void startBlocking();

		void accept();
		void processMessage(Socket&& socket);



	};
}



