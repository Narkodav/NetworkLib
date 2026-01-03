#include "../include/RestfulServer.h"

namespace Network::HTTP
{
	std::string RestfulServer::getErrorResponse(std::string_view uri,
		Request::Method method,
		Response::StatusCode code,
		const std::string_view message) {		
		return Json::Value{{
			{"error", {{"code", static_cast<int64_t>(code)}, {"message", message}, 
				{"path", uri}, {"method", Request::methodToString(method)}}},
			{"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
		}}.stringify();
	}

	std::unique_ptr<Response> RestfulServer::createNotFoundResponse(Request& req, Request::Method method) {
		auto resp = std::make_unique<Response>();
		resp->setVersion("HTTP/1.1");
		resp->setStatusCode(Response::StatusCode::NotFound);
		resp->setStatusMessage("Not Found");
		auto& headers = resp->getHeaders();
		headers.set(Message::Headers::Standard::ContentType, "application/json");
		headers.set(Message::Headers::Standard::Server, m_core.getName());
		auto body = std::make_unique<StringBody>();
		auto json = getErrorResponse(req.getUri(), method, Response::StatusCode::NotFound, "Resource not found");
		headers.set(Message::Headers::Standard::ContentLength, std::to_string(json.size()));
		body->write(json.data(), json.size());
		resp->setBody(std::move(body));
		addCORSHeaders(*resp);
		return resp;
	}

	std::unique_ptr<Response> RestfulServer::createPreflightCorsResponse(Request& req, Request::Method method)
	{
		auto resp = std::make_unique<Response>();		
		addCORSHeaders(*resp);
		addSuccessfulHeaders(*resp);
		auto& headers = resp->getHeaders();
		headers.set(Message::Headers::Standard::ContentLength, "0");
		return resp;
	}

	std::unique_ptr<Response> RestfulServer::handleGet(Request& req) {
		return handleGeneric(req, Request::Method::Get, &RestfulServer::createNotFoundResponse);
	}

	std::unique_ptr<Response> RestfulServer::handleConnect(Request& req) {
		return handleGeneric(req, Request::Method::Connect, &RestfulServer::createNotFoundResponse);
	}

	std::unique_ptr<Response> RestfulServer::handleDelete(Request& req) {
		return handleGeneric(req, Request::Method::Delete, &RestfulServer::createNotFoundResponse);
	}

	std::unique_ptr<Response> RestfulServer::handleHead(Request& req) {
		return handleGeneric(req, Request::Method::Head, &RestfulServer::createNotFoundResponse);
	}

	std::unique_ptr<Response> RestfulServer::handleOptions(Request& req) {
		return handleGeneric(req, Request::Method::Options, &RestfulServer::createPreflightCorsResponse);
	}

	std::unique_ptr<Response> RestfulServer::handlePatch(Request& req) {
		return handleGeneric(req, Request::Method::Patch, &RestfulServer::createNotFoundResponse);
	}

	std::unique_ptr<Response> RestfulServer::handlePost(Request& req) {
		return handleGeneric(req, Request::Method::Post, &RestfulServer::createNotFoundResponse);
	}

	std::unique_ptr<Response> RestfulServer::handlePut(Request& req) {
		return handleGeneric(req, Request::Method::Put, &RestfulServer::createNotFoundResponse);
	}

	std::unique_ptr<Response> RestfulServer::handleTrace(Request& req) {
		return handleGeneric(req, Request::Method::Trace, &RestfulServer::createNotFoundResponse);
	}

	std::unique_ptr<Response> RestfulServer::handleUnknown(Request& req) {
		auto resp = std::make_unique<Response>();
		resp->setVersion("HTTP/1.1");
		resp->setStatusCode(Response::StatusCode::MethodNotAllowed);
		resp->setStatusMessage("Method Not Allowed");
		auto& headers = resp->getHeaders();
		headers.set(Message::Headers::Standard::ContentType, "application/json");
		headers.set(Message::Headers::Standard::Server, m_core.getName());
		auto body = std::make_unique<StringBody>();
		auto json = getErrorResponse(req.getUri(), Request::Method::Unknown, Response::StatusCode::MethodNotAllowed, "Method not allowed");
		headers.set(Message::Headers::Standard::ContentLength, std::to_string(json.size()));
		body->write(json.data(), json.size());
		resp->setBody(std::move(body));
		addCORSHeaders(*resp);
		return resp;
	}

	void RestfulServer::addCORSHeaders(Response& resp) {
		auto& headers = resp.getHeaders();
		headers.set(Message::Headers::Standard::AccessControlAllowOrigin, m_corsOptions.allowedOrigins);
		headers.set(Message::Headers::Standard::AccessControlAllowMethods, m_corsOptions.allowedMethods);
		headers.set(Message::Headers::Standard::AccessControlAllowHeaders, m_corsOptions.allowedHeaders);
	}

	void RestfulServer::addSuccessfulHeaders(Response& resp) {
		resp.setVersion("HTTP/1.1");
		resp.setStatusCode(Network::HTTP::Response::StatusCode::Ok);
		resp.setStatusMessage("OK");
		auto& headers = resp.getHeaders();
		headers.set(Message::Headers::Standard::Server, m_core.getName());
	}
}