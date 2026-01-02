#pragma once
#include "Server.h"
#include "Session.h"
#include <span>
#include <string_view>
#include <stack>

#include "JsonParser/Value.h"


namespace Network::HTTP
{
    class RestfulServer
    {
        using Handler = std::function<std::unique_ptr<Response>(Request&,
            std::span<std::string_view>)>;

        struct Node {
            std::unordered_map<std::string, Node*, Detail::TransparentStringHash, Detail::TransparentStringEqual> children;
			Node* parameterChild = nullptr;
            std::array<Handler, static_cast<size_t>(Request::Method::Count)> handlers;
        };

    private:
        Server m_core;
		Node* m_root;
		IOContext m_ioContext;

    public:

		RestfulServer(int port, std::string_view name) :
            m_core(m_ioContext, port, name),
            m_root(nullptr) {

            m_core.setHandler(Request::Method::Get, [this](Request& req) { return handleGet(req); });
            m_core.setHandler(Request::Method::Connect, [this](Request& req) { return handleConnect(req); });
            m_core.setHandler(Request::Method::Delete, [this](Request& req) { return handleDelete(req); });
            m_core.setHandler(Request::Method::Head, [this](Request& req) { return handleHead(req); });
            m_core.setHandler(Request::Method::Options, [this](Request& req) { return handleOptions(req); });
            m_core.setHandler(Request::Method::Patch, [this](Request& req) { return handlePatch(req); });
            m_core.setHandler(Request::Method::Post, [this](Request& req) { return handlePost(req); });
            m_core.setHandler(Request::Method::Put, [this](Request& req) { return handlePut(req); });
            m_core.setHandler(Request::Method::Trace, [this](Request& req) { return handleTrace(req); });
            m_core.setHandler(Request::Method::Unknown, [this](Request& req) { return handleUnknown(req); });
        }
        
        void addEndpoint(std::string path,
            Request::Method method,
            Handler&& handler) {
            if (path[0] != '/') path = "/" + path;
            registerHandle(m_root, path, method, std::move(handler));
        }

        void start() {
            m_core.startBlocking();
		}

    private:

        std::unique_ptr<Response> handleGet(Request& req);
        std::unique_ptr<Response> handleConnect(Request& req);
        std::unique_ptr<Response> handleDelete(Request& req);
        std::unique_ptr<Response> handleHead(Request& req);
        std::unique_ptr<Response> handleOptions(Request& req);
        std::unique_ptr<Response> handlePatch(Request& req);
        std::unique_ptr<Response> handlePost(Request& req);
        std::unique_ptr<Response> handlePut(Request& req);
        std::unique_ptr<Response> handleTrace(Request& req);
        std::unique_ptr<Response> handleUnknown(Request& req);
        
        std::string getErrorResponse(std::string_view uri,
            Request::Method method,
            Response::StatusCode code,
			const std::string_view message);
        std::unique_ptr<Response> createNotFoundResponse(Request& req, Request::Method method);

        bool findHandler(Node* node,
            std::string_view path,
            Request::Method method,
            Handler& outHandler,
            std::vector<std::string_view>& outParams) {
            if (path[0] == '/') path = path.substr(1);
            while (path != "")
            {
                auto segment = getPathSegment(path, 0);
                auto it = node->children.find(segment);
                if (it != node->children.end()) {
                    path = path.substr(segment.length() + 1);
					node = it->second;
                }
                else if (node->parameterChild == nullptr) {
                    return false;
                }
                else
                {
                    outParams.push_back(segment);
                    path = path.substr(segment.length() + 1);
                    node = node->parameterChild;
                }
            }
            outHandler = node->handlers[static_cast<size_t>(method)];
            return true;
		}

        std::string_view getPathSegment(std::string_view path, size_t startPos) {
            size_t endPos = path.find('/', startPos);
            if (endPos == std::string::npos) {
                endPos = path.length();
            }
            std::string_view segment = path.substr(startPos, endPos - startPos);
            return segment;
        }
        
        void registerHandle(Node*& node, std::string_view path, Request::Method method, Handler&& handler) {            
            if (node == nullptr) node = new Node();

            while (path != "")
            {
                auto segment = getPathSegment(path, 1);

                if (segment[0] == '{' && segment.back() == '}' ||
                    segment[0] == ':') {
                    if (node->parameterChild == nullptr) {
                        node->parameterChild = new Node();
                    }
                    node = node->parameterChild;
                }
                else
                {
                    auto it = node->children.find(segment);
                    if (it == node->children.end()) {
                        it = node->children.emplace_hint(it,
                            segment, new Node()
                        );
                    }
                    node = it->second;
                }
                path = path.substr(segment.length() + 1);
            }
            node->handlers[static_cast<size_t>(method)] = std::move(handler);
		}

        void deleteTree() {
            std::stack<Node*> nodeStack;
            Node* current = m_root; 
            Node* buffer;

            while (current != nullptr) {
                if(!current->children.empty()) {					
                    nodeStack.push(current);
					auto it = current->children.begin();
                    buffer = it->second;
                    current = buffer;
					buffer->children.erase(it);
                    continue;
                }
                else if (current->parameterChild != nullptr) {
                    nodeStack.push(current);
                    buffer = current->parameterChild;
                    current = buffer;
					buffer->parameterChild = nullptr;
                }
                else {
                    delete current;
                    if (nodeStack.empty()) {
                        current = nullptr;
                    }
                    else {
                        current = nodeStack.top();
                        nodeStack.pop();
                    }
				}
            }        
        }
    };
}
