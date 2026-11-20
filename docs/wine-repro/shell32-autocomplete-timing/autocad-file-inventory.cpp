/* Original, AI-assisted (Codex), LGPL-2.1-or-later. */
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef _WIN32
# include <windows.h>
# include <bcrypt.h>
#else
# include <openssl/evp.h>
#endif

namespace fs = std::filesystem;
using Digest = std::array<unsigned char, 32>;

#ifdef _WIN32
class Sha256
{
public:
    Sha256()
    {
        check(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0));
        if (BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) < 0)
        {
            BCryptCloseAlgorithmProvider(algorithm, 0);
            throw std::runtime_error("SHA-256 initialization failed");
        }
    }

    ~Sha256()
    {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(algorithm, 0);
    }

    void update(unsigned char *data, unsigned int size) { check(BCryptHashData(hash, data, size, 0)); }
    void finish(Digest &digest) { check(BCryptFinishHash(hash, digest.data(), digest.size(), 0)); }

private:
    static void check(NTSTATUS status)
    {
        if (status < 0) throw std::runtime_error("SHA-256 failed");
    }

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
};
#else
class Sha256
{
public:
    Sha256()
    {
        if (!context) throw std::runtime_error("SHA-256 initialization failed");
        check(EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr));
    }

    void update(unsigned char *data, unsigned int size) { check(EVP_DigestUpdate(context.get(), data, size)); }
    void finish(Digest &digest) { check(EVP_DigestFinal_ex(context.get(), digest.data(), nullptr)); }

private:
    static void check(int result)
    {
        if (result != 1) throw std::runtime_error("SHA-256 failed");
    }

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context{EVP_MD_CTX_new(), EVP_MD_CTX_free};
};
#endif

bool is_link(const fs::path &path)
{
#ifdef _WIN32
    const DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) throw std::runtime_error("file attributes unavailable");
    return (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
    return fs::is_symlink(path);
#endif
}

bool is_program_file(const fs::path &path)
{
    static const std::array<std::string, 14> extensions{
        ".exe", ".dll", ".arx", ".dbx", ".crx", ".hdi", ".ocx", ".tlb",
        ".mui", ".config", ".json", ".manifest", ".xml", ".dwt"};
    std::string extension = path.extension().u8string();
    for (char &c : extension) c = std::tolower(static_cast<unsigned char>(c));
    for (const auto &allowed : extensions)
        if (allowed == extension) return true;
    return false;
}

void emit_file(const fs::path &path, const std::string &name)
{
    if (name.find_first_of("\t\r\n") != std::string::npos)
        throw std::runtime_error("unsupported filename");
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot open an inventory file");
    Sha256 hash;
    std::array<unsigned char, 65536> buffer;
    uintmax_t size = 0;
    while (file)
    {
        file.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
        const auto count = static_cast<unsigned int>(file.gcount());
        if (count) hash.update(buffer.data(), count);
        size += count;
    }
    if (!file.eof() || file.bad() || size != fs::file_size(path))
        throw std::runtime_error("file read failed or size changed during inventory");
    Digest digest;
    hash.finish(digest);
    std::cout << "file\t" << name << '\t' << size << '\t' << std::hex << std::setfill('0');
    for (unsigned char byte : digest) std::cout << std::setw(2) << static_cast<unsigned int>(byte);
    std::cout << std::dec << '\n';
}

void scan_tree(const fs::path &root, const std::string &relative)
{
    for (const auto &entry : fs::directory_iterator(root))
    {
        if (is_link(entry.path())) continue;
        const std::string name = relative + '/' + entry.path().filename().u8string();
        if (entry.is_directory()) scan_tree(entry.path(), name);
        else if (entry.is_regular_file() && is_program_file(entry.path())) emit_file(entry.path(), name);
    }
}

bool emit_root(const fs::path &path, const std::string &name)
{
    const bool present = fs::is_directory(path) && !is_link(path);
    std::cout << "root\t" << name << '\t' << (present ? "present" : "missing") << '\n';
    return present;
}

void scan_runtime(const fs::path &path, const std::string &name)
{
    if (!emit_root(path, name)) return;
    for (const auto &entry : fs::directory_iterator(path))
    {
        const auto version = entry.path().filename().u8string();
        if (version.rfind("10.", 0) == 0 && entry.is_directory() && !is_link(entry.path()))
        {
            std::cout << "root\t" << name << '/' << version << "\tpresent\n";
            scan_tree(entry.path(), name + '/' + version);
        }
    }
}

int inventory(const fs::path &program_files)
{
    if (!fs::is_directory(program_files)) throw std::runtime_error("Program Files directory unavailable");
    std::cout << "winum-autocad-file-inventory-v1\n";
    const auto acad = program_files / "Autodesk" / "AutoCAD 2027";
    if (emit_root(acad, "acad")) scan_tree(acad, "acad");
    for (const std::string name : {"AcJab.dll", "AdMigrator.xml", "Support/AcCommandWeight.xml", "dotnet/dotnet.exe"})
    {
        const auto path = program_files / name;
        if (fs::is_regular_file(path) && !is_link(path))
            emit_file(path, name.rfind("dotnet/", 0) == 0 ? name : "program-files/" + name);
    }
    for (const std::string name : {"host/fxr", "shared/Microsoft.NETCore.App", "shared/Microsoft.WindowsDesktop.App"})
        scan_runtime(program_files / "dotnet" / name, "dotnet/" + name);
    std::cout << "end\n" << std::flush;
    if (!std::cout) throw std::runtime_error("inventory output failed");
    return 0;
}

#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
    try
    {
        if (argc == 2) return inventory(argv[1]);
#ifdef _WIN32
        if (argc == 1)
        {
            const wchar_t *program_files = _wgetenv(L"ProgramW6432");
            if (!program_files) program_files = _wgetenv(L"ProgramFiles");
            if (program_files) return inventory(program_files);
        }
#endif
        std::cerr << "Usage: autocad-file-inventory PROGRAM_FILES_DIRECTORY\n";
    }
    catch (const std::exception &)
    {
        std::cerr << "Inventory failed while reading the selected directories or hashing a file.\n";
    }
    return 2;
}
