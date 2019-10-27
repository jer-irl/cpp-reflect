#include "CodeGeneration.hpp"
#include <fstream>
#include <iostream>

namespace CppReflect {

void generateRegistrationFile(clang::ASTUnit& astUnit, const std::string& outPath, const std::string& inPath) {
    std::ofstream os{outPath};
    std::cout << outPath << std::endl;

    os << "// Test\n";
    os.flush();
}

} // namespace CppReflect
