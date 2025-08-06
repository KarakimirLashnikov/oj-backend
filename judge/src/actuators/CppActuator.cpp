#include "actuators/CppActuator.hpp"
#include "utilities/ExecutionResult.hpp"
#include <kafel.h>
#include <seccomp.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mount.h>
#include "Logger.hpp"
#include <sys/prctl.h>
#include <asm/unistd.h>
#include <linux/seccomp.h>
#include "FileException.hpp"
#include "SystemException.hpp"
#include <cstdio>
#include <cstring>

constexpr static const char* KAFEL_POLICY { R"KAFEL(
    POLICY oj_sandbox {
        // ==================== 允许的系统调用 ====================
        ALLOW {
            // 基本 I/O ( 仅允许 stdin/stdout/stderr )
            read {
                fd == 0  // stdin
            }
            write {
                fd == 1 || fd == 2  // stdout/stderr
            }
            close {
                fd == 0 || fd == 1 || fd == 2  // 允许关闭标准流
            }

            // 内存管理（允许基本的堆栈操作）
            brk,
            mmap {
                prot == PROT_READ | PROT_WRITE,
                flags == MAP_PRIVATE | MAP_ANONYMOUS
            },
            munmap,
            mprotect {
                prot == PROT_READ | PROT_WRITE | PROT_EXEC
            }

            // 基本进程控制（仅允许正常退出）
            exit,
            exit_group,
            rt_sigreturn,

            // 时间测量（允许获取运行时间）
            clock_gettime,
            gettimeofday,

            // 架构相关(x86_64 常见调用)
            arch_prctl,
            set_tid_address,
            futex {
                // 仅允许 FUTEX_WAIT 和 FUTEX_WAKE(防止滥用)
                op == FUTEX_WAIT || op == FUTEX_WAKE
            }
        }

        // ==================== 拒绝的危险系统调用 ====================
        DENY {
            // 文件系统(禁止所有文件访问)
            open,
            openat,
            creat,
            unlink,
            mkdir,
            rmdir,
            chmod,
            chown,
            rename,
            symlink,
            readlink,
            stat,
            lstat,
            fstat,

            // 网络(禁止所有网络访问)
            socket,
            connect,
            bind,
            listen,
            accept,
            sendto,
            recvfrom,
            sendmsg,
            recvmsg,
            shutdown,

            // 进程控制(禁止 fork/exec)
            fork,
            vfork,
            clone,
            execve,
            kill,
            tkill,
            tgkill,

            // 系统信息（禁止获取敏感信息）
            getpid,
            getppid,
            getuid,
            getgid,
            geteuid,
            getegid,
            getrandom,
            sysinfo,

            // 其他危险调用
            ptrace,
            prctl,
            seccomp,
            ioctl  // 禁止设备控制
        }
    }

    // 应用策略，默认拒绝并终止进程
    USE oj_sandbox DEFAULT KILL
)KAFEL" };

static std::once_flag ONCE_FLAG{};
static sock_fprog SHARED_SECCOMP{};
static bool IS_SYSTEM_INITIALIZED { false };
constexpr static const char *CGROUP_ROOT{ "/sys/fs/cgroup" };

namespace Judge
{
    void CppActuator::initSystem()
    {
        std::call_once(::ONCE_FLAG, [](){  
            // 1. 检查/proc目录
            Exceptions::checkFileExists("/proc");
            Exceptions::checkFileWritable("/proc");
            
            // 2. 检查cgroup根目录
            Exceptions::checkFileExists(::CGROUP_ROOT);
            Exceptions::checkFileWritable(::CGROUP_ROOT);
            
            // 3. 编译Kafel策略
            kafel_ctxt_t ctxt = kafel_ctxt_create();
            kafel_set_input_string(ctxt, ::KAFEL_POLICY);
            if (kafel_compile(ctxt, &::SHARED_SECCOMP) != 0) {
                const char* error_msg = kafel_error_msg(ctxt);
                kafel_ctxt_destroy(&ctxt);
                throw Exceptions::SystemException(-1, error_msg);
            }
            kafel_ctxt_destroy(&ctxt);
            
            // 4. 注册退出时释放资源的函数
            atexit([]() {
                if (::SHARED_SECCOMP.filter) {
                    free(::SHARED_SECCOMP.filter);
                    ::SHARED_SECCOMP.filter = nullptr;
                }
            });
            
            ::IS_SYSTEM_INITIALIZED = true;
            LOG_INFO("Global initialization completed");
        });
    }

    CppActuator::CppActuator()
    {
        if (!IS_SYSTEM_INITIALIZED) 
            throw Exceptions::SystemException(-1, "system not initialized");

        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1) {
            throw Exceptions::SystemException(errno, "prctl(PR_SET_NO_NEW_PRIVS)");
        }
    }

    void CppActuator::joinCgroup(const fs::path& cgroup_path) 
    {
        Exceptions::checkFileExists(cgroup_path);
        Exceptions::checkFileWritable(cgroup_path);
        std::ofstream procs_file(cgroup_path / "cgroup.procs");
        if (!procs_file) {
            throw Exceptions::makeSystemException("Open cgroup.procs in CppActuator::joinCgroup");
        }
        procs_file << getpid() << std::endl;
    }

    ExecutionResult CppActuator::execute(fs::path exe_path, const ResourceLimits &limits, std::string_view stdin_data)
    {
        Exceptions::checkFileExists(exe_path.string());
        Exceptions::checkFileExecutable(exe_path.string());

        // 使用匿名管道 , 0 为父端， 1 为子端
        int stdout_pipe[2], stderr_pipe[2], stdin_pipe[2];
        if (pipe(stdout_pipe)) throw Exceptions::SystemException(errno, "pipe stdout failed");
        if (pipe(stderr_pipe)) throw Exceptions::SystemException(errno, "pipe stderr failed");
        if (pipe(stdin_pipe)) throw Exceptions::SystemException(errno, "pipe stdin failed");

        auto start_time{ std::chrono::steady_clock::now() };
        
        pid_t child_pid{ fork() };
        if (child_pid < 0) {
            throw Exceptions::SystemException(errno, "fork failed");
        }
        else if (child_pid == 0) {
            // 子进程逻辑
            runChildProcess(stdin_pipe, stdout_pipe, stderr_pipe, exe_path, limits);
            dprintf(STDERR_FILENO, "execv failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        // 父进程逻辑
        close(stdin_pipe[0]);   // 关闭父的输入管道
        close(stdout_pipe[1]);  // 关闭子的输出管道
        close(stderr_pipe[1]);  // 关闭子的输出管道
        
        if (!stdin_data.empty()) {
            size_t total_written = 0;
            while (total_written < stdin_data.size()) {
                ssize_t written = write(stdin_pipe[1], stdin_data.data() + total_written, stdin_data.size() - total_written);
                if (written < 0) {
                    // 释放资源，准备退出
                    kill(child_pid, SIGKILL);
                    waitpid(child_pid, nullptr, 0);
                    close(stdin_pipe[1]);
                    close(stdout_pipe[0]);
                    close(stderr_pipe[0]);
                    throw Exceptions::makeSystemException(
                        "write to child's stdin failed in CppActuator::execute: " + std::string(strerror(errno)));
                }
                total_written += written;
            }
        }
        // 正常关闭 stdin 管道
        close(stdin_pipe[1]);
        
        // 创建并设置cgroup
        try {
            fs::path cgroup_path{ createCgroupForProcess(child_pid, limits) };
            ExecutionResult result;
            this->monitorChild(child_pid, 
                            stdout_pipe[0], // 子向父端输入
                            stderr_pipe[0], // 子向父端输入
                            start_time, 
                            result, 
                            limits,
                            cgroup_path);
                             // 关闭管道
            close(stdout_pipe[0]);
            close(stderr_pipe[0]);

            return result;
        } catch (const Exceptions::FileException& fe) {
            close(stdout_pipe[0]);
            close(stderr_pipe[0]);
            throw fe;
        }
    }

    void CppActuator::runChildProcess(int stdin_pipe[2], int stdout_pipe[2], int stderr_pipe[2], 
                                      const fs::path& exe_path, const ResourceLimits& limits)
    {
        // 关闭父端管道
        close(stdin_pipe[1]);   // 关闭父端写
        close(stdout_pipe[0]);  // 关闭父端读
        close(stderr_pipe[0]);  // 关闭父端读
        
        // 重定向标准IO
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        
        // 关闭多余描述符
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
        
        // 设置安全环境
        setupChildEnv(limits);
        
        // 执行程序
        char* args[] = {const_cast<char*>(exe_path.filename().c_str()), nullptr};
        execv(exe_path.c_str(), args);
    }

    fs::path CppActuator::createCgroupForProcess(pid_t pid, const ResourceLimits& limits)
    {
        fs::path cgroup_root = ::CGROUP_ROOT;
        std::string cgroup_name = "judge_" + std::to_string(getpid()) + "_" + std::to_string(pid);
        fs::path cgroup_path = cgroup_root / cgroup_name;
        
        // 创建cgroup目录
        if (!fs::create_directories(cgroup_path)) {
            throw Exceptions::makeFileException(cgroup_path.c_str(), "create cgroup for child process");
        }
        
        // 设置cgroup限制
        setupCgroupLimits(limits, cgroup_path);
        
        // 将进程加入cgroup
        std::ofstream procs_file(cgroup_path / "cgroup.procs");
        if (!procs_file) {
            throw Exceptions::makeFileException(cgroup_path / "cgroup.procs", "open cgroup.procs");
        }
        procs_file << pid << std::endl;
        procs_file.close();
        
        LOG_DEBUG("Cgroup setup completed for PID: {}", pid);
        return cgroup_path;
    }

    void CppActuator::setupCgroupLimits(const ResourceLimits &limits, const fs::path& cgroup_path)
    {
        // 设置CPU时间限制
        std::ofstream cpu_max_file(cgroup_path / "cpu.max");
        if (cpu_max_file) {
            cpu_max_file << (static_cast<uint64_t>(limits.cpu_time_limit_s * 1000000)) << " 1000000" << std::endl;
        } else {
            LOG_WARN("Failed to write cpu.max");
        }

        // 设置内存限制
        std::ofstream(cgroup_path / "memory.max") << (limits.memory_limit_kb * 1024) << std::endl;
        std::ofstream(cgroup_path / "memory.swap.max") << "0" << std::endl;
        std::ofstream(cgroup_path / "memory.oom.group") << "1" << std::endl;
    }

    void CppActuator::setupChildEnv(const ResourceLimits &limits)
    {
        // 1. 设置资源限制
        struct rlimit stack_limit{
            .rlim_cur = static_cast<rlim_t>(limits.stack_limit_kb * 1024),
            .rlim_max = static_cast<rlim_t>(limits.stack_limit_kb * 1024)};
        setrlimit(RLIMIT_STACK, &stack_limit);

        signal(SIGPIPE, SIG_IGN);
        
        // 2. 创建命名空间
        if (unshare(CLONE_NEWNS | CLONE_NEWUTS) != 0) {
            perror("unshare failed");
            exit(EXIT_FAILURE);
        }
        
        // 3. 挂载 proc
        if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, nullptr) != 0) {
            perror("mount proc failed");
            exit(EXIT_FAILURE);
        }
        
        // 4. 加载全局安全策略
        if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &::SHARED_SECCOMP) == -1) {
            perror("prctl(PR_SET_SECCOMP) failed");
            exit(EXIT_FAILURE);
        }

        // 5. 降低权限
        if (setgid(1000) != 0 || setuid(1000) != 0) {
            perror("Failed to drop privileges");
            exit(EXIT_FAILURE);
        }
    }

    void CppActuator::monitorChild(pid_t pid,
                          int stdout_fd,
                          int stderr_fd,
                          const TimeStamp &start_time,
                          ExecutionResult &result,
                          const ResourceLimits &limits,
                          const fs::path& cgroup_path)
    {
        LOG_DEBUG("Monitoring child process: {}", pid);
        
        // 使用poll同时监控进程退出和输出
        struct pollfd fds[3] = {
            {.fd = stdout_fd, .events = POLLIN},
            {.fd = stderr_fd, .events = POLLIN},
            {.fd = -1, .events = 0}
        };
        
        std::string stdout_data, stderr_data;
        char buffer[4096];
        bool process_exited = false;
        int status = 0;
        
        const auto timeout_time = start_time + std::chrono::microseconds(
            static_cast<int64_t>(limits.wall_time_limit_s * 1000000));
        bool timeout{ false };
        
        while (!process_exited) {
            // 检查超时
            const auto now = std::chrono::steady_clock::now();
            if (now >= timeout_time) {
                LOG_WARN("Process {} timed out, sending SIGKILL", pid);
                kill(pid, SIGKILL);
                timeout = true;
                break;
            }
            
            // 计算剩余时间（毫秒）
            int timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeout_time - now).count();
            timeout_ms = std::max(10, std::min(100, timeout_ms));  // 限制在10-100ms
            
            int ret = poll(fds, 2, timeout_ms);
            if (ret < 0) {
                if (errno == EINTR) continue; // 被信号打断，忽略
                LOG_ERROR("poll error: {}", strerror(errno));
                break;
            }
            
            // 读取可用输出
            if (fds[0].revents & POLLIN) {
                ssize_t n = read(stdout_fd, buffer, sizeof(buffer));
                if (n > 0) {
                    stdout_data.append(buffer, n);
                } else if (n == 0) {
                    fds[0].fd = -1;  // 标记为关闭
                }
            }
            
            if (fds[1].revents & POLLIN) {
                ssize_t n = read(stderr_fd, buffer, sizeof(buffer));
                if (n > 0) {
                    stderr_data.append(buffer, n);
                } else if (n == 0) {
                    fds[1].fd = -1;  // 标记为关闭
                }
            }
            
            // 检查进程状态
            pid_t wpid = waitpid(pid, &status, WNOHANG);
            if (wpid == pid) {
                process_exited = true;
                LOG_DEBUG("Child process exited: PID={}", pid);
            } else if (wpid == -1) {
                if (errno == ECHILD) {
                    process_exited = true;
                    LOG_DEBUG("Child process not found, assuming exited");
                } else {
                    LOG_ERROR("waitpid error: {}", strerror(errno));
                }
            }
            
            // 检查是否所有输出都已关闭
            if (fds[0].fd == -1 && fds[1].fd == -1) {
                LOG_DEBUG("All output pipes closed");
                break;
            }
        }
        
        // 确保回收子进程
        if (!process_exited) {
            LOG_DEBUG("Waiting for child process to exit...");
            waitpid(pid, &status, 0);
        }
        
        // 读取剩余输出
        while (true) {
            ssize_t n = read(stdout_fd, buffer, sizeof(buffer));
            if (n <= 0) break;
            stdout_data.append(buffer, n);
        }
        
        while (true) {
            ssize_t n = read(stderr_fd, buffer, sizeof(buffer));
            if (n <= 0) break;
            stderr_data.append(buffer, n);
        }
        
        result.stdout = std::move(stdout_data);
        result.stderr = std::move(stderr_data);
        result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        result.signal = WIFSIGNALED(status) ? WTERMSIG(status) : 0;
        
        try {
            // 读取内存峰值
            if (fs::exists(cgroup_path / "memory.peak")) {
                std::ifstream mem_file(cgroup_path / "memory.peak");
                if (mem_file) {
                    uint64_t mem_peak = 0;
                    mem_file >> mem_peak;
                    result.memory_kb = mem_peak / 1024;
                }
            }
            
            // 读取CPU时间统计
            if (fs::exists(cgroup_path / "cpu.stat")) {
                std::ifstream cpu_stat(cgroup_path / "cpu.stat");
                std::string line;
                while (std::getline(cpu_stat, line)) {
                    if (line.find("usage_usec") == 0) {
                        uint64_t cpu_us;
                        if (sscanf(line.c_str(), "usage_usec %lu", &cpu_us) == 1) {
                            result.cpu_time_us = cpu_us;
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to read cgroup stats: {}", e.what());
            result.cpu_time_us = 0;
            result.memory_kb = 0;
            result.status = TestStatus::INTERNAL_ERROR;
            cleanupCgroup(cgroup_path);
            return;
        }
        
        result.create_at = start_time;
        result.finish_at = std::chrono::steady_clock::now();

        // 判断退出原因
        if (timeout) {
            result.status = TestStatus::TIME_LIMIT_EXCEEDED;
        } else if (result.memory_kb >= limits.memory_limit_kb) {
            result.status = TestStatus::MEMORY_LIMIT_EXCEEDED;
        } else if (result.signal != 0 || result.exit_code != 0) {
            result.status = TestStatus::RUNTIME_ERROR;
        } else {
            result.status = TestStatus::UNKNOWN; // 未判定 , 默认
        }

        // 清理cgroup
        cleanupCgroup(cgroup_path);
    }

    void CppActuator::cleanupCgroup(const fs::path& cgroup_path)
    {
        try {
            Exceptions::checkFileExists(cgroup_path);
            
            // 递归删除子cgroup
            std::vector<fs::path> subdirs;
            for (const auto& entry : fs::directory_iterator(cgroup_path)) {
                if (entry.is_directory()) {
                    subdirs.push_back(entry.path());
                }
            }
            for (const auto& subdir : subdirs) {
                cleanupCgroup(subdir);
            }

            // 使用 rmdir 删除cgroup目录
            int attempts = 0;
            while (attempts < 3) {
                if (rmdir(cgroup_path.c_str()) == 0) {
                    LOG_DEBUG("Cgroup removed: {}", cgroup_path.string());
                    return;
                }
                
                if (errno == ENOTEMPTY) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    ++attempts;
                } else {
                    LOG_ERROR("rmdir failed: {} [{}]", strerror(errno), cgroup_path.string());
                    return;
                }
            }
            LOG_ERROR("Failed to remove cgroup after 3 attempts: {}", cgroup_path.string());
        } catch (const std::exception& e) {
            LOG_ERROR("Cgroup cleanup error: {}", e.what());
        }
    }
}