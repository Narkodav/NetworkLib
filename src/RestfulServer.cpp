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
		return resp;
	}

	std::unique_ptr<Response> RestfulServer::handleGet(Request& req) {
		Handler handler;
		std::vector<std::string_view> params;
		if(findHandler(m_root, req.getUri(), Request::Method::Get, handler, params)) return handler(req, params);
		return createNotFoundResponse(req, Request::Method::Get);
	}

	std::unique_ptr<Response> RestfulServer::handleConnect(Request& req) {
		Handler handler;
		std::vector<std::string_view> params;
		if(findHandler(m_root, req.getUri(), Request::Method::Connect, handler, params)) return handler(req, params);
		return createNotFoundResponse(req, Request::Method::Connect);
	}

	std::unique_ptr<Response> RestfulServer::handleDelete(Request& req) {
		Handler handler;
		std::vector<std::string_view> params;
		if(findHandler(m_root, req.getUri(), Request::Method::Delete, handler, params)) return handler(req, params);
		return createNotFoundResponse(req, Request::Method::Delete);
	}

	std::unique_ptr<Response> RestfulServer::handleHead(Request& req) {
		Handler handler;
		std::vector<std::string_view> params;
		if(findHandler(m_root, req.getUri(), Request::Method::Head, handler, params)) return handler(req, params);
		return createNotFoundResponse(req, Request::Method::Head);
	}

	std::unique_ptr<Response> RestfulServer::handleOptions(Request& req) {
		Handler handler;
		std::vector<std::string_view> params;
		if(findHandler(m_root, req.getUri(), Request::Method::Options, handler, params)) return handler(req, params);
		return createNotFoundResponse(req, Request::Method::Options);
	}

	std::unique_ptr<Response> RestfulServer::handlePatch(Request& req) {
		Handler handler;
		std::vector<std::string_view> params;
		if(findHandler(m_root, req.getUri(), Request::Method::Patch, handler, params)) return handler(req, params);
		return createNotFoundResponse(req, Request::Method::Patch);
	}

	std::unique_ptr<Response> RestfulServer::handlePost(Request& req) {
		Handler handler;
		std::vector<std::string_view> params;
		if(findHandler(m_root, req.getUri(), Request::Method::Post, handler, params)) return handler(req, params);
		return createNotFoundResponse(req, Request::Method::Post);
	}

	std::unique_ptr<Response> RestfulServer::handlePut(Request& req) {
		Handler handler;
		std::vector<std::string_view> params;
		if(findHandler(m_root, req.getUri(), Request::Method::Put, handler, params)) return handler(req, params);
		return createNotFoundResponse(req, Request::Method::Put);
	}

	std::unique_ptr<Response> RestfulServer::handleTrace(Request& req) {
		Handler handler;
		std::vector<std::string_view> params;
		if(findHandler(m_root, req.getUri(), Request::Method::Trace, handler, params)) return handler(req, params);
		return createNotFoundResponse(req, Request::Method::Trace);
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
		return resp;
	}
}