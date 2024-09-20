#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <string.h>
#include <iostream>

#ifdef _WIN32
#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winsock.h>
#include <Windows.h>
#include "Lib.h"
#include "Logger.h"
#include "XMLFile.h"
#include "TestClass.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <iconv.h>
#include <wchar.h>
#include "Lib.h"
#include "XMLFile.h"
#include "TestClass.h"
#include "Logger.h"
#include <cstdio>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#endif

#include <vector>
#include <queue>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <deque>
#include <memory>