#include "application.h"

#include <stdexcept>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    bool bBenchmarkEnabled = false;
    for (int i = 0; i < argc; i++){
        if(std::string_view(argv[i]) == "--benchmark"){
            bBenchmarkEnabled = true;
        }
    }
    try
    {
        Application app(bBenchmarkEnabled);
        app.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
