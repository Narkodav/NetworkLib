#include "include/RestfulServer.h"
#include "include/Session.h"

int main()
{
	Network::HTTP::RestfulServer server(8080, "RestfulServer");

	server.addEndpoint("/hello/{name}", Network::HTTP::Request::Method::Get,
		[](Network::HTTP::Request& req, std::span<std::string_view> params) -> std::unique_ptr<Network::HTTP::Response>
		{
			auto response = std::make_unique<Network::HTTP::Response>();
			response->setVersion("HTTP/1.1");
			response->setStatusCode(Network::HTTP::Response::StatusCode::Ok);
			response->setStatusMessage("OK");
			auto& headers = response->getHeaders();
			headers.set(Network::HTTP::Message::Headers::Standard::ContentType, "application/json");
			headers.set(Network::HTTP::Message::Headers::Standard::Server, "RestfulServer");
			auto body = std::make_unique<Network::HTTP::StringBody>();
			std::string name = params.size() > 0 ? std::string(params[0]) : "world";
			auto json = Json::Value{
				{"message", "Hello, " + name}
			}.stringify();
			headers.set(Network::HTTP::Message::Headers::Standard::ContentLength, std::to_string(json.size()));
			body->write(json.data(), json.size());
			response->setBody(std::move(body));
			return response;
		});
	server.start();

	return 0;
}