#pragma once
#include <string>
#include <vector>
#include <algorithm>

#include "include/RestfulServer.h"
#include "JsonParser/Value.h"

class TaskManager
{    
    struct Task {
        using Id = uint64_t;
        Id id;
        std::string title;
        bool completed = false;
    };

    std::vector<Task> tasks;
    Task::Id nextId = 1;


    public:
    TaskManager() = default;

    void registerRoutes(Network::HTTP::RestfulServer& server) {
        server.addEndpoint("/tasks", Network::HTTP::Request::Method::Get, 
            [this](Network::HTTP::Request& req, 
            std::span<std::string_view> params) -> std::unique_ptr<Network::HTTP::Response> {
                    auto tasksJson = getTasks().stringify();
                    std::cout << tasksJson << std::endl;
					auto response = std::make_unique<Network::HTTP::Response>();
                    response->getHeaders().set(Network::HTTP::Message::Headers::Standard::ContentType,
                        "application/json");
                    response->getHeaders().set(Network::HTTP::Message::Headers::Standard::ContentLength,
                        std::to_string(tasksJson.size()));
                    auto body = std::make_unique<Network::HTTP::StringBody>();
                    body->write(tasksJson.data(), tasksJson.size());
                    response->setBody(std::move(body));
					return response;
            });
        server.addEndpoint("/tasks", Network::HTTP::Request::Method::Post,
            [this](Network::HTTP::Request& req,
                std::span<std::string_view> params) -> std::unique_ptr<Network::HTTP::Response> {
                    auto length = std::stoi(req.getHeaders().get(
                        Network::HTTP::Message::Headers::Standard::ContentLength));
                    auto& reqBody = req.getBody();
                    std::string bodyStr(length, '\0');
                    reqBody->read(bodyStr.data(), 0, length);
                    auto bodyJson = Json::Value::parse<Json::StrictContainerParser>(bodyStr);
                    auto title = bodyJson["title"].asString();
                    auto id = addTask(title);
                    std::string responseJson = Json::Value{
                        {"id", static_cast<int64_t>(id)},
                        {"title", title},
                        {"completed", false}
					}.stringify();
                    std::cout << responseJson << std::endl;
                    auto response = std::make_unique<Network::HTTP::Response>();
                    response->getHeaders().set(Network::HTTP::Message::Headers::Standard::ContentType,
                        "application/json");
                    response->getHeaders().set(Network::HTTP::Message::Headers::Standard::ContentLength,
                        std::to_string(responseJson.size()));
                    auto respBody = std::make_unique<Network::HTTP::StringBody>();
                    respBody->write(responseJson.data(), responseJson.size());
                    response->setBody(std::move(respBody));
                    return response;
            });
        server.addEndpoint("/tasks/{id}/toggle", Network::HTTP::Request::Method::Put, 
            [this](Network::HTTP::Request& req,
                std::span<std::string_view> params) -> std::unique_ptr<Network::HTTP::Response> {
                    auto id = std::stoull(std::string(params[0]));
                    completeTask(id);
                    auto response = std::make_unique<Network::HTTP::Response>();
                    response->getHeaders().set(Network::HTTP::Message::Headers::Standard::ContentType,
                        "application/json");
                    response->getHeaders().set(Network::HTTP::Message::Headers::Standard::ContentLength, "0");
                    return response;
            });
        server.addEndpoint("/tasks/{id}", Network::HTTP::Request::Method::Delete, 
            [this](Network::HTTP::Request& req,
                std::span<std::string_view> params) -> std::unique_ptr<Network::HTTP::Response> {
                    auto id = std::stoull(std::string(params[0]));
                    removeTask(id);
                    auto response = std::make_unique<Network::HTTP::Response>();
                    response->getHeaders().set(Network::HTTP::Message::Headers::Standard::ContentType,
                        "application/json");
                    response->getHeaders().set(Network::HTTP::Message::Headers::Standard::ContentLength, "0");
                    return response;
            });
    }

private:
    Json::Value getTasks() const {
        Json::Value tasksJson = Json::Value::array();
        auto& tasksArray = tasksJson.asArray();
        tasksArray.reserve(tasks.size());
        for (const auto& task : tasks) {
            Json::Value taskJson = {
                {"id", static_cast<int64_t>(task.id)},
                {"title", task.title},
                {"completed", task.completed}
            };
            tasksArray.push_back(taskJson);
        }
        return tasksJson;
	}

    Task::Id addTask(const std::string& title) {
        tasks.push_back(Task{ nextId++, title, false });
        return tasks.back().id;
	}

    void completeTask(Task::Id id) {
        for (auto& task : tasks) {
            if (task.id == id) {
                task.completed = !task.completed;  // Toggle instead of just setting true
                return;
            }
        }
	}

    void removeTask(Task::Id id) {
        tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
            [id](const Task& task) { return task.id == id; }), tasks.end());
	}
};

