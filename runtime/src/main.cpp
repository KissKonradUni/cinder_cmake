#include "cinder.hpp"

int main(int argc, char* argv[]) {
    cinder::Application app;
    app.printDebugInfo();
    app.run(argc, argv);
    return 0;
}