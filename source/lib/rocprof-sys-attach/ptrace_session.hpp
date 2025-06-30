#include <cstddef>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cstdint>
#include <sys/ptrace.h>
#include <dlfcn.h>

#ifndef ROCP_TRACE
#define ROCP_TRACE std::cout
#endif

#ifndef ROCP_ERROR
#define ROCP_ERROR std::cerr
#endif

namespace rocprofsys {
namespace attach {

class PTraceSession
{
    public:
    explicit PTraceSession(size_t);
    ~PTraceSession();

    bool attach();
    bool detach();
    bool simple_mmap(void*& addr, size_t length);
    bool simple_munmap(void*& addr, size_t length);
    
    bool write(size_t addr, const std::vector<uint8_t>& data, size_t size);
    bool read(size_t addr, std::vector<uint8_t>& data, size_t size);
    bool swap(size_t addr, const std::vector<uint8_t>& in_data, std::vector<uint8_t>& out_data, size_t size);

    size_t get_pid();

    bool call_function(const std::string& library, const std::string& symbol);
    bool call_function(const std::string& library, const std::string& symbol, void* first);
    bool call_function(const std::string& library, const std::string& symbol, void* first, void* second);
    bool call_function(const std::string& library, const std::string& symbol, void* first, void* second, unsigned long long* ret);
    
    unsigned long long open_library(const std::string& library);
    unsigned long long open_library(const std::string& library, int flag);

    bool stop();
    bool cont();

    private:

    bool find_library(void*& addr, int inpid, const std::string& library);
    bool find_symbol(void*& addr, const std::string& library, const std::string& symbol);

    std::unordered_map<std::string, void*> target_library_addrs;
    std::unordered_map<std::string, void*> target_symbol_addrs;

    const size_t pid;
    bool attached;
};
}
}

