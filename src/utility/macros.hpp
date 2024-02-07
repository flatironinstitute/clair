#pragma once
#include <iostream>

// A few simple macros

#define AS_STRING(...) AS_STRING2(__VA_ARGS__)
#define AS_STRING2(...) #__VA_ARGS__

#define REQUIRES(...) __attribute__((enable_if(__VA_ARGS__, AS_STRING2(__VA_ARGS__))))

// DEBUG PRINTING
#define PRINT(X) std::cerr << AS_STRING(X) << " = " << X << "      at " << __FILE__ << ":" << __LINE__ << '\n'

#define EXPECTS(X)                                                                                                                                   \
  if (!(X)) {                                                                                                                                        \
    std::cerr << "\033[1;31mERROR : \033[1;35mPrecondition " << AS_STRING(X) << " violated at " << __FILE__ << ":" << __LINE__ << "\033[0m\n";       \
    std::terminate();                                                                                                                                \
  }
#define ASSERT(X)                                                                                                                                    \
  if (!(X)) {                                                                                                                                        \
    std::cerr << "\033[1;31mERROR : \033[1;35mAssertion " << AS_STRING(X) << " violated at " << __FILE__ << ":" << __LINE__ << "\033[0m\n";          \
    std::terminate();                                                                                                                                \
  }
#define ENSURES(X)                                                                                                                                   \
  if (!(X)) {                                                                                                                                        \
    std::cerr << "\033[1;31mERROR : \033[1;35mPostcondition " << AS_STRING(X) << " violated at " << __FILE__ << ":" << __LINE__ << "\033[0m\n";      \
    std::terminate();                                                                                                                                \
  }

#define EXPECTS_WITH_MESSAGE(X, ...)                                                                                                                 \
  if (!(X)) {                                                                                                                                        \
    std::cerr << "\033[1;31mERROR : \033[1;35mPrecondition " << AS_STRING(X) << " violated at " << __FILE__ << ":" << __LINE__ << "\033[0m\n";       \
    std::cerr << "Error message : " << __VA_ARGS__ << std::endl;                                                                                     \
    std::terminate();                                                                                                                                \
  }
#define ASSERT_WITH_MESSAGE(X, ...)                                                                                                                  \
  if (!(X)) {                                                                                                                                        \
    std::cerr << "\033[1;31mERROR : \033[1;35mAssertion " << AS_STRING(X) << " violated at " << __FILE__ << ":" << __LINE__ << "\033[0m\n";          \
    std::cerr << "Error message : " << __VA_ARGS__ << std::endl;                                                                                     \
    std::terminate();                                                                                                                                \
  }
#define ENSURES_WITH_MESSAGE(X, ...)                                                                                                                 \
  if (!(X)) {                                                                                                                                        \
    std::cerr << "\033[1;31mERROR : \033[1;35mPostcondition " << AS_STRING(X) << " violated at " << __FILE__ << ":" << __LINE__ << "\033[0m\n";      \
    std::cerr << "Error message : " << __VA_ARGS__ << std::endl;                                                                                     \
    std::terminate();                                                                                                                                \
  }
