#include "CodeGeneration.hpp"
#include <clang/Basic/MemoryBufferCache.h>
#include <clang/Serialization/ASTWriter.h>
#include <clang/Tooling/Tooling.h>
#include <fstream>
#include <iostream>

namespace CppReflect {

void dumpAstFile(clang::ASTUnit& ast, const std::string& targetPath) {
    llvm::SmallVector<char, 0> astOutVector;
    llvm::BitstreamWriter streamWriter{astOutVector};
    llvm::SmallVector<char, 0> astOutBuffer;
    clang::MemoryBufferCache memBufCache;
    llvm::ArrayRef<std::shared_ptr<clang::ModuleFileExtension>> moduleFileExtensions;
    clang::ASTWriter astWriter{streamWriter, astOutBuffer, memBufCache, moduleFileExtensions};
    astWriter.WriteAST(ast.getSema(), "/tmp/output.ast", nullptr, "");
    std::cout << astOutVector.size() << std::endl;
    std::ofstream astOutFileStream{targetPath};
    astOutFileStream.write(astOutVector.data(), astOutVector.size());
    std::cout << "HERE\n";
    std::cout << targetPath << "\n";
}

} // namespace CppReflect

int main(int argc, const char* argv[]) {
    std::string inputFilePath;
    std::string outCppPath;
    std::string outAstPath;
    std::vector<std::string> compilerArgs;

    std::uint_fast32_t argIdx = 1;
    bool consumeRest = false;
    while (argIdx < argc) {
        if (consumeRest) {
            compilerArgs.emplace_back(argv[argIdx]);
        } else if (std::string{argv[argIdx]} == "-ocpp") {
            ++argIdx;
            outCppPath = argv[argIdx];
        } else if (std::string{argv[argIdx]} == "-oast") {
            ++argIdx;
            outAstPath = argv[argIdx];
        } else if (argv[argIdx] == "--") {
            consumeRest = true;
        } else {
            inputFilePath = argv[argIdx];
        }
        ++argIdx;
    }

    const std::string sourceName = inputFilePath.substr(inputFilePath.rfind('/') + 1);

    std::ifstream sourceStream{inputFilePath};
    std::string code{std::istreambuf_iterator<char>{sourceStream}, std::istreambuf_iterator<char>{}};
    const std::unique_ptr<clang::ASTUnit> ast =
            clang::tooling::buildASTFromCodeWithArgs(code, compilerArgs, inputFilePath);

    CppReflect::dumpAstFile(*ast, outAstPath);

    CppReflect::generateRegistrationFile(*ast, outCppPath, inputFilePath);

    return 0;
}
