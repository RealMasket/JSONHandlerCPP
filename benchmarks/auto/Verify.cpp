#include "Compare.hpp"
#include "../reservation/Data.hpp"
#include <iostream>
int main(int argc,char** argv) {
    try {
        AutoCheck::selfTest();
        if (argc==2 && std::string_view(argv[1])=="--self-test") { std::cout<<"Auto arithmetic/semantic self-test passed\n"; return 0; }
        if (argc<2) throw std::invalid_argument("Usage: AutoVerify INPUT... | --self-test");
        for (int i=1;i<argc;++i) {
            const auto source=MatrixData::read(argv[i]);
            if (!AutoCheck::compare(source)) throw std::runtime_error("Expected valid Auto fixture");
        }
        std::cout<<"Exact token fields, values and diagnostics match both fixed modes\n";
    } catch (const std::exception& ex) { std::cerr<<ex.what()<<'\n'; return 1; }
}
