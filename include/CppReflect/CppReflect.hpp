#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/TextDiagnosticBuffer.h>
#include <clang/Serialization/ASTReader.h>
#include <clang/Tooling/JSONCompilationDatabase.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/TargetSelect.h>
#include <cstdio>
#include <filesystem>
#include <type_traits>
#include <unistd.h>
#include <unordered_map>

namespace CppReflect {
namespace Details {

/// From driver.cpp
inline std::string GetExecutablePath(const char* Argv0, bool CanonicalPrefixes) {
    if (!CanonicalPrefixes) {
        llvm::SmallString<128> ExecutablePath(Argv0);
        // Do a PATH lookup if Argv0 isn't a valid path.
        if (!llvm::sys::fs::exists(ExecutablePath))
            if (llvm::ErrorOr<std::string> P = llvm::sys::findProgramByName(ExecutablePath))
                ExecutablePath = *P;
        return ExecutablePath.str();
    }

    // This just needs to be some symbol in the binary; C++ doesn't
    // allow taking the address of ::main however.
    void* P = (void*) (intptr_t) GetExecutablePath;
    return llvm::sys::fs::getMainExecutable(Argv0, P);
}

class CompilationDatabaseEntry {
public:
    explicit CompilationDatabaseEntry(const char* bufferBegin, const char* buffer);

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
    explicit ASTEntry(const std::string& translationUnitPath, const char* astBufferBegin, const char* astBufferEnd);

    clang::ASTContext& getASTContext(const clang::tooling::CompileCommand& compileCommand) {
        if (not compilerInstance_.hasASTContext()) {
            // Copied from clang cc1 main
            llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diagID{new clang::DiagnosticIDs{}};

            // Buffer diagnostics from argument parsing so that we can output them using a
            // well formed diagnostic object.
            llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> diagOpts = new clang::DiagnosticOptions{};
            clang::TextDiagnosticBuffer* diagsBuffer = new clang::TextDiagnosticBuffer;
            clang::DiagnosticsEngine diags(diagID, &*diagOpts, diagsBuffer);
            const std::vector<std::string>& args = compileCommand.CommandLine;
            const char** const argArray = new const char*[args.size()];
            for (std::uint_fast8_t i = 0; i < args.size(); ++i) {
                argArray[i] = args[i].data();
            }
            bool success = clang::CompilerInvocation::CreateFromArgs(
                    compilerInstance_.getInvocation(), argArray, argArray + args.size() - 1, diags);

            // Infer the builtin include path if unspecified.
            if (compilerInstance_.getHeaderSearchOpts().UseBuiltinIncludes
                && compilerInstance_.getHeaderSearchOpts().ResourceDir.empty())
                compilerInstance_.getHeaderSearchOpts().ResourceDir =
                        clang::CompilerInvocation::GetResourcesPath(argArray[0], (void *)(intptr_t) GetExecutablePath);

            // Create the actual diagnostics engine.
            compilerInstance_.createDiagnostics();
            if (!compilerInstance_.hasDiagnostics())
                std::abort();

            // Set an error handler, so that any LLVM backend diagnostics go through our
            // error handler.
            //llvm::install_fatal_error_handler(LLVMErrorHandler, static_cast<void*>(&Clang->getDiagnostics()));

            diagsBuffer->FlushDiagnostics(compilerInstance_.getDiagnostics());
            if (!success)
                std::abort();

            // Execute the frontend actions.
            /*
            {
                llvm::TimeTraceScope TimeScope("ExecuteCompiler", StringRef(""));
                Success = ExecuteCompilerInvocation(Clang.get());
            }
            */

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
            return astEntries_.at(absolutePath)
                    .get()
                    .getASTContext(compilationDatabase_->getCompileCommands(absolutePath)[0]);
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

inline ASTEntry::ASTEntry(const std::string& translationUnitPath, const char* astBufferBegin, const char* astBufferEnd)
        : astBufferBegin_{astBufferBegin}, astBufferSize_{static_cast<std::size_t>(astBufferEnd - astBufferBegin)} {
    Registry::getSharedInstance().registerASTEntry(translationUnitPath, *this);
}

inline CompilationDatabaseEntry::CompilationDatabaseEntry(const char* bufferBegin, const char* bufferEnd)
        : bufferBegin_{bufferBegin}, bufferSize_{static_cast<std::size_t>(bufferEnd - bufferBegin)} {
    Registry::getSharedInstance().registerCompilationDatabaseEntry(*this);
}

} // namespace Details

inline const clang::ASTContext& astForTranslationUnit(const std::string& path) {
    return Details::Registry::getSharedInstance().getASTContextForFile(path);
}

} // namespace CppReflect
