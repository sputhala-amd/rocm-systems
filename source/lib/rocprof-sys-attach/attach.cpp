// MIT License
//
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#include <memory>
#include "ptrace_session.hpp"
#include <signal.h>
#include "attach.hpp"

namespace common = ::rocprofsys::common;

namespace
{
#define ROCPROFILER_CALL(result, msg)                                                              \
    {                                                                                              \
        rocprofiler_status_t ROCPROFILER_VARIABLE(CHECKSTATUS, __LINE__) = result;                 \
        if(ROCPROFILER_VARIABLE(CHECKSTATUS, __LINE__) != ROCPROFILER_STATUS_SUCCESS)              \
        {                                                                                          \
            std::string status_msg =                                                               \
                rocprofiler_get_status_string(ROCPROFILER_VARIABLE(CHECKSTATUS, __LINE__));        \
            std::cerr << "[" #result "][" << __FILE__ << ":" << __LINE__ << "] " << msg            \
                      << " failed with error code " << ROCPROFILER_VARIABLE(CHECKSTATUS, __LINE__) \
                      << ": " << status_msg << "\n"                                                \
                      << std::flush;                                                               \
            std::stringstream errmsg{};                                                            \
            errmsg << "[" #result "][" << __FILE__ << ":" << __LINE__ << "] " << msg " failure ("  \
                   << status_msg << ")";                                                           \
            throw std::runtime_error(errmsg.str());                                                \
        }                                                                                          \
    }

    std::unique_ptr<rocprofsys::attach::PTraceSession> ptrace_session;
    
}


extern "C" 
{

int
rocprofsys_attach(size_t pid, std::vector<char*> env) 
{  
    std::cout << "Process id of host: " << getpid() << std::endl;
    std::cout << "Attachment library called for pid " << pid << std::endl;
    ptrace_session = std::make_unique<rocprofsys::attach::PTraceSession>(pid);
    ROCP_TRACE << "Attempting attachment to pid " << pid << std::endl;
    if (!ptrace_session->attach())
    {
        ROCP_ERROR << "Attachment failed to pid " << pid << std::endl;
    }
    ROCP_TRACE << "Attachment success to pid " << pid << std::endl;

    // Environment_buffer is a null-character delimited list of name value pairs.
    // Each name and value is delimited separately.
    // The first 4 bytes contain an uint32_t count of pairs

    std::vector<uint8_t> environment_buffer(4);
    {
        uint32_t var_count = 0;
        for (char* var: env)
        {
            if (strncmp("ROCPROF", var, 7) != 0)
            {
                continue;
            }
            var_count++;
            ROCP_TRACE << "Adding to environment buffer: " << var << std::endl;
            while(*var != '=')
            {
                environment_buffer.emplace_back(*var++);
            }
            environment_buffer.emplace_back(0);
            var++;
            while(*var)
            {
                environment_buffer.emplace_back(*var++);
            }
            environment_buffer.emplace_back(0);
        }

        const uint8_t* var_count_bytes = reinterpret_cast<uint8_t*>(&var_count);
        std::copy(var_count_bytes, var_count_bytes + 4, environment_buffer.data());
    }

    // now, allocate a buffer to store the environment variables
    void* environment_buffer_addr = nullptr;
    ptrace_session->simple_mmap(environment_buffer_addr, environment_buffer.size());
    ROCP_TRACE << "mmap'd in target process at " << environment_buffer_addr << std::endl;

    // write to that buffer
    ptrace_session->stop();
    ptrace_session->write(reinterpret_cast<size_t>(environment_buffer_addr), environment_buffer, environment_buffer.size());
    ptrace_session->cont();
    ROCP_TRACE << "wrote environment buffer to target process" << std::endl;

    // register the API table for rocprofiler. We have to explictly call this here.
    ptrace_session->call_function("librocprofiler-register.so", "rocprofiler_register_invoke_all_registrations", nullptr);

    // dlopen librocprof-sys-dl.so on the target 
    // first we write the library name into the target memory
    const char* libname_cstring = "librocprof-sys-dl.so";
    std::vector<uint8_t> libname_buffer(
        reinterpret_cast<const uint8_t*>(libname_cstring),
        reinterpret_cast<const uint8_t*>(libname_cstring) + strlen(libname_cstring) + 1 // +1 for the null terminator
    );
    void* libname_buffer_addr = nullptr;
    ptrace_session->simple_mmap(libname_buffer_addr, libname_buffer.size());

    // write that to buffer
    ptrace_session->stop();
    ptrace_session->write(reinterpret_cast<size_t>(libname_buffer_addr), libname_buffer, libname_buffer.size());
    ptrace_session->cont();
    ROCP_TRACE << "wrote library name to target process" << std::endl;

    //now we dlopen
    ptrace_session->call_function("libc.so", "dlopen", libname_buffer_addr, (void*) 1); 
    // execute the attach function with the buffer addr as parameter
    ptrace_session->call_function("librocprof-sys-dl.so", "rocprofsys_dl_attach", environment_buffer_addr);
    //TODO: call other APIs from the register library to properly restore the program state?
    ptrace_session->stop();
    ptrace_session->detach();

    std::cout << "Entering into attach mode. Press any key to detach.\n";
    std::cin.get();

    kill(pid, 10);
    // ptrace_session->attach();
    // ptrace_session->call_function("librocprof-sys-dl.so", "rocprofsys_dl_detach", nullptr);
    // ptrace_session->stop();
    // ptrace_session->detach();
    // ptrace_session.reset();
    return 0;
}

void
rocprofsys_detach()
{
    //TODO: call other APIs from the register library to properly restore the program state?
    ptrace_session->call_function("librocprof-sys-dl.so", "rocprofsys_dl_detach", nullptr);
    ptrace_session->stop();
    ptrace_session->detach();
    ptrace_session.reset();
}

}