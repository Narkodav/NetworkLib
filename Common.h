#pragma once
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <memory>
#include <string>
#include <array>

//Vendor

#include <Multithreading/ThreadPool.h>