#pragma once

#include "Engine/Core/Object.hpp"
#include <BS_thread_pool.hpp>

inline SharedPtr<BS::thread_pool> GThreadPool = MakeShared<BS::thread_pool>();
