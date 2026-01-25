#pragma once

#include <string>
#include <typeinfo>
#include <memory>
#include <cstdlib>

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>

/**
 * @brief get the demangled type string for the specified type
 * 
 * @return demangled type string
 */
template <typename T>
std::string getTypeString() {
    int status;
    // Use unique_ptr with std::free as the deleter
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status),
        std::free
    };

    return (status == 0) ? res.get() : typeid(T).name();
}
#else
// Fallback for compilers like MSVC (which often returns demangled names by default)
template <typename T>
std::string getTypeString() {
    return typeid(T).name();
}
#endif