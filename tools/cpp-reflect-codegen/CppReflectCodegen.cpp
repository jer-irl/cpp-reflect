#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

namespace CppReflect {
namespace CLI {
namespace cl = llvm::cl;
cl::OptionCategory optionCategory{"cpp-reflect-codegen options"};
cl::opt<std::string> outputFilename{"o", cl::desc("Output filename"), cl::Required};
} // namespace CLI
} // namespace CppReflect

int main(int argc, const char* argv[]) {
    namespace cl = llvm::cl;
    cl::ParseCommandLineOptions(argc, argv);
    clang::tooling::CommonOptionsParser optionsParser{argc, argv, CppReflect::CLI::optionCategory};

    clang::tooling::ClangTool clangTool{optionsParser.getCompilations(), optionsParser.getSourcePathList()};

    return 0;
}
