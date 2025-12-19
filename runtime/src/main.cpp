#include <print>

#include "cinder.hpp"

int main() {
    cinder::debugVersionInfo();
    std::println("Hello from Runtime!");
    return 0;
}