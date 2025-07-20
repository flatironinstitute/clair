#pragma once

#if __has_include(<generator>) // Check if <generator> exists
#include <generator>
using std::generator;
#else
#include "cppcoro/generator.hpp"
using cppcoro::generator;
#endif
