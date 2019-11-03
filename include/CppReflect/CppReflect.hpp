#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Serialization/ASTReader.h>
#include <clang/Tooling/JSONCompilationDatabase.h>
#include <cstdio>
#include <filesystem>
#include <type_traits>
#include <unistd.h>
#include <unordered_map>

namespace CppReflect {
namespace Details {

class CompilationDatabaseEntry {
public:
    explicit CompilationDatabaseEntry(const char* bufferBegin, std::size_t bufferSize);

    std::unique_ptr<clang::tooling::JSONCompilationDatabase> getCompilationDb() {
        llvm::StringRef buffer{bufferBegin_, bufferSize_};
        std::string err;
        std::unique_ptr<clang::tooling::JSONCompilationDatabase> result =
                clang::tooling::JSONCompilationDatabase::loadFromBuffer(
                        buffer, err, clang::tooling::JSONCommandLineSyntax::AutoDetect);
        if (not result) {
            // TODO(Jeremy)
            std::terminate();
        }
        return result;
    }

private:
    const char* const bufferBegin_;
    const std::size_t bufferSize_;
};

class ASTEntry {
public:
    explicit ASTEntry(const std::string& translationUnitPath, const char* astBufferBegin, std::size_t astBufferSize);

    clang::ASTContext& getASTContext(const clang::tooling::CompileCommand& compileCommand) {
        if (not compilerInstance_.hasASTContext()) {
            auto invocationSPtr = std::make_shared<clang::CompilerInvocation>();
            const std::vector<std::string>& args = compileCommand.CommandLine;
            const char** const argArray = new const char*[args.size()];
            for (std::uint_fast8_t i = 0; i < args.size(); ++i) {
                argArray[i] = args[i].data();
            }
            clang::CompilerInvocation::CreateFromArgs(*invocationSPtr, argArray, argArray + args.size() - 1, diags_);
            compilerInstance_.setInvocation(invocationSPtr);
            compilerInstance_.createPreprocessor(clang::TranslationUnitKind::TU_Complete);
            compilerInstance_.createASTContext();

            std::FILE* astFile = writeASTFile();
            std::string astFilePath = getFilePath(astFile);

            clang::ASTReader reader{compilerInstance_.getPreprocessor(),
                                    &compilerInstance_.getASTContext(),
                                    compilerInstance_.getPCHContainerReader(),
                                    {}};
            clang::ASTReader::ASTReadResult result = reader.ReadAST(
                    astFilePath, clang::ASTReader::ModuleKind::MK_MainFile, clang::SourceLocation{},
                    clang::ASTReader::LoadFailureCapabilities::ARR_None);
            std::fclose(astFile);
        }
        return compilerInstance_.getASTContext();
    }

private:
    std::FILE* writeASTFile() const {
        std::FILE* result = std::tmpfile();
        std::size_t totalWritten = 0;
        const char* ptr = astBufferBegin_;
        while (totalWritten < astBufferSize_) {
            std::size_t written = std::fwrite(ptr, 1, astBufferSize_ - totalWritten, result);
            totalWritten += written;
            ptr += written;
        }
        return result;
    }

    static std::string getFilePath(std::FILE* file) {
        std::string procPath = "/proc/self/fd/" + std::to_string(fileno(file));
        char buffer[1024];
        std::size_t rc = readlink(procPath.data(), buffer, 1024);
        if (rc == -1) {
            // TODO(Jeremy)
            std::terminate();
        }
        buffer[rc] = '\0';
        return std::string{buffer};
    }

    const char* const astBufferBegin_;
    const std::size_t astBufferSize_;
    clang::CompilerInstance compilerInstance_;

    clang::DiagnosticsEngine diags_;
};

class Registry {
    friend ASTEntry;
    friend CompilationDatabaseEntry;

public:
    static Registry& getSharedInstance() {
        static Registry registry;
        return registry;
    }

    const clang::ASTContext& getASTContextForFile(const std::string& path) {
        if (not compilationDatabase_) {
            if (not compilationDatabaseEntry_) {
                std::abort();
            }
            compilationDatabase_ = compilationDatabaseEntry_->getCompilationDb();
        }
        std::string absolutePath = qualifyPath(compilationDatabase_->getAllFiles(), path);

        if (astEntries_.count(absolutePath) == 1) {
            return astEntries_.at(path).get().getASTContext(compilationDatabase_->getCompileCommands(absolutePath)[0]);
        }
        std::abort();
    }

private:
    void registerASTEntry(const std::string& translationUnitPath, ASTEntry& entry) {
        astEntries_.emplace(translationUnitPath, std::reference_wrapper<ASTEntry>{entry});
    }

    void registerCompilationDatabaseEntry(CompilationDatabaseEntry& entry) { compilationDatabaseEntry_ = &entry; }

    template<
            typename ContainerT,
            std::enable_if_t<std::is_same_v<typename ContainerT::value_type, std::string>, int> = 0>
    static std::string qualifyPath(const ContainerT& fullPaths, const std::string& searchPathStr) {
        std::filesystem::path searchPath{searchPathStr};
        if (searchPath.is_absolute()) {
            return searchPathStr;
        }
        for (const std::string& absPathStr : fullPaths) {
            std::filesystem::path absPath{absPathStr};
            auto absPathStartIter = absPath.begin();
            while (absPathStartIter != absPath.end()) {
                auto absPathIter = absPathStartIter;
                auto searchPathIter = searchPath.begin();
                while (*absPathIter == *searchPathIter) {
                    ++absPathIter;
                    ++searchPathIter;
                    if (absPathIter == absPath.end() and searchPathIter == searchPath.end()) {
                        return absPathStr;
                    }
                }

                ++absPathStartIter;
            }
        }
        std::abort();
    }

    std::unordered_map<std::string, std::reference_wrapper<ASTEntry>> astEntries_;
    CompilationDatabaseEntry* compilationDatabaseEntry_ = nullptr;
    std::unique_ptr<clang::tooling::JSONCompilationDatabase> compilationDatabase_;
};

inline ASTEntry::ASTEntry(const std::string& translationUnitPath, const char* astBufferBegin, std::size_t astBufferSize)
        : astBufferBegin_{astBufferBegin}, astBufferSize_{astBufferSize}, diags_{nullptr, nullptr, nullptr} {
    Registry::getSharedInstance().registerASTEntry(translationUnitPath, *this);
}

inline CompilationDatabaseEntry::CompilationDatabaseEntry(const char* bufferBegin, std::size_t bufferSize)
        : bufferBegin_{bufferBegin}, bufferSize_{bufferSize} {
    Registry::getSharedInstance().registerCompilationDatabaseEntry(*this);
}

} // namespace Details

inline const clang::ASTContext& astForTranslationUnit(const std::string& path) {
    return Details::Registry::getSharedInstance().getASTContextForFile(path);
}

} // namespace CppReflect
