#pragma once

#include <windows.h>

#ifdef OPVD_PCH_GLFW

// GLFW
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "GLFW/glfw3ext.h"

#if OURO_PLATFORM_WIN
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#pragma pop_macro("APIENTRY")
#endif // OURO_PLATFORM_WIN

#endif // OPVD_PCH_GLFW


#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <exception>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <limits>
#include <locale>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

// CLI11
#include "CLI11.hpp"

// github.com/martinus/unordered_dense
#include "ankerl/unordered_dense.h"

// spdlog
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>  // support for loading levels from the environment variable
#include <spdlog/fmt/ostr.h> // support for user defined types
#include <spdlog/sinks/basic_file_sink.h>

// physx
#include "pvd/PxPvd.h"
#include "PsFileBuffer.h"
