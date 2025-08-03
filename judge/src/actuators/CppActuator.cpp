#include "actuators/CppActuator.hpp"
#include "utilities/ExecutionResult.hpp"
#include <seccomp.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mount.h>
#include "Logger.hpp"

namespace Judge
{
    ExecutionResult CppActuator::execute(fs::path exe_path, const ResourceLimits &limits, std::string_view stdin_data)
    {
        // 输出重定向
        bp::pipe stdout_pipe, stderr_pipe, stdin_pipe;
        
        auto start_time = std::chrono::steady_clock::now();
        pid_t child_pid = fork();
        if (child_pid < 0) {
            throw std::system_error(std::error_code(errno, std::system_category()), "fork error");
        }
        else if (child_pid == 0) {
            // 子进程
            this->setupChildEnv(limits);
            this->lanchProcess(stdin_pipe, stdout_pipe, stderr_pipe, exe_path);
            exit(EXIT_FAILURE); // 确保子进程异常退出后 , 父进程可以继续运行
        }
        else {
            // 父进程
            if (!stdin_data.empty()) {
                bp::opstream in(stdin_pipe);
                in.write(stdin_data.data(), stdin_data.size());
                in.flush();
                in.close();  // 显式关闭输入
            }
            stdin_pipe.close();  // 确保关闭
            
            // 延迟关闭输出管道
            stdout_pipe.close();
            stderr_pipe.close();
            
            // 将管道连接到流对象
            bp::ipstream stdout_stream(stdout_pipe);
            bp::ipstream stderr_stream(stderr_pipe);

            fs::path cgroup_root = "/sys/fs/cgroup";
            if (!fs::exists(cgroup_root)) {
                cgroup_root = "/sys/fs/cgroup/unified";
            }
            fs::path cgroup_path = cgroup_root / ("judge_" + std::to_string(getpid()) + "_" + std::to_string(child_pid));
            
            LOG_DEBUG("Creating cgroup at: {}", cgroup_path.string());
            this->setupCgroups(child_pid, limits, cgroup_path);

            ExecutionResult result;
            this->monitorChild(child_pid, stdout_stream, stderr_stream, start_time, result, limits, cgroup_path);

            // 在返回结果前启动清理线程
            std::thread([this, cgroup_path]() {
                // 等待1秒确保资源统计完成
                std::this_thread::sleep_for(std::chrono::seconds(1));
                this->cleanupCgroups(cgroup_path);
            }).detach();

            return result;
        }
    }

    void CppActuator::setupChildEnv(const ResourceLimits &limits)
    {
        // 设置资源限制
        struct rlimit stack_limit{
            .rlim_cur = static_cast<rlim_t>(limits.stack_limit_kb * 1024),
            .rlim_max = static_cast<rlim_t>(limits.stack_limit_kb * 1024)};
        setrlimit(RLIMIT_STACK, &stack_limit);

        signal(SIGPIPE, SIG_IGN);
        // Seccomp 白名单
        // scmp_filter_ctx filter_ctx = seccomp_init(SCMP_ACT_KILL);
        // if (!filter_ctx) {
        //     perror("seccomp_init failed");
        //     exit(EXIT_FAILURE);
        // }
        // this->allowSyscalls(filter_ctx);
        // if (seccomp_load(filter_ctx) != 0) {
        //     perror("seccomp_load failed");
        //     exit(EXIT_FAILURE);
        // }
        // seccomp_release(filter_ctx);

        // 创建隔离命名空间
        if (unshare(CLONE_NEWNS | CLONE_NEWNET) != 0) {
            perror("unshare failed");
            exit(EXIT_FAILURE);
        }

        // 挂载proc文件系统
        if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, nullptr) != 0) {
            perror("mount proc failed");
            // 非致命错误，继续执行
        }
    }

    void CppActuator::allowSyscalls(scmp_filter_ctx ctx)
    {
        // 基础系统调用白名单
        const std::vector<int> allowed_syscalls = {
            SCMP_SYS(read), SCMP_SYS(write), SCMP_SYS(close),
            SCMP_SYS(fstat), SCMP_SYS(mmap), SCMP_SYS(mprotect),
            SCMP_SYS(munmap), SCMP_SYS(rt_sigaction), SCMP_SYS(rt_sigprocmask),
            SCMP_SYS(ioctl), SCMP_SYS(access), SCMP_SYS(arch_prctl),
            SCMP_SYS(brk), SCMP_SYS(exit_group), SCMP_SYS(readlink),
            SCMP_SYS(sysinfo), SCMP_SYS(writev), SCMP_SYS(lseek),
            SCMP_SYS(clock_gettime), SCMP_SYS(getrandom),
            SCMP_SYS(open), SCMP_SYS(openat), SCMP_SYS(exit)  // 添加更多必要调用
        };

        for (int syscall : allowed_syscalls) {
            if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, syscall, 0) != 0) {
                fprintf(stderr, "Failed to add rule for syscall %d\n", syscall);
            }
        }

        // 特殊规则：限制文件描述符
        // seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 1, SCMP_A0(SCMP_CMP_LE, 2));
        // seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 1, SCMP_A0(SCMP_CMP_LE, 2));
        // seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 1, SCMP_A0(SCMP_CMP_LE, 2));
    }

    void CppActuator::lanchProcess(bp::pipe &stdin_pipe,
                                   bp::pipe &stdout_pipe,
                                   bp::pipe &stderr_pipe,
                                   const fs::path &exe_path)
    {
        // 重定向标准流
        dup2(stdin_pipe.native_source(), STDIN_FILENO);
        dup2(stdout_pipe.native_sink(), STDOUT_FILENO);
        dup2(stderr_pipe.native_sink(), STDERR_FILENO);
        // ==================================

        // 关闭所有管道描述符
        stdin_pipe.close();
        stdout_pipe.close();
        stderr_pipe.close();

        // 关闭所有可能的打开文件描述符
        for (int fd = sysconf(_SC_OPEN_MAX); fd > 2; fd--) {
            close(fd);
        }

        if (setgid(1000) != 0 || setuid(1000) != 0) {
            perror("降低权限失败");
            exit(EXIT_FAILURE);
        }

        // 执行程序
        execl(exe_path.c_str(), exe_path.filename().c_str(), nullptr);
        perror("execl failed");
        exit(EXIT_FAILURE);
    }

    void CppActuator::setupCgroups(pid_t pid, const ResourceLimits &limits, const fs::path& cgroup_path)
    {
        // 创建cgroup目录
        if (!fs::create_directories(cgroup_path)) {
            throw std::runtime_error("Failed to create cgroup directory: " + cgroup_path.string());
        }
        
        std::ofstream(cgroup_path / "cgroup.type") << "threaded\n";

        // 将进程加入cgroup
        std::ofstream procs_file(cgroup_path / "cgroup.procs");
        if (!procs_file) {
            // 添加详细错误信息
            throw std::runtime_error("无法写入 cgroup.procs: " + 
                std::string(strerror(errno)) + " at " + 
                (cgroup_path / "cgroup.procs").string());
        }
        procs_file << pid << std::endl;
        procs_file.close();

        // 设置CPU时间限制
        std::ofstream cpu_max_file(cgroup_path / "cpu.max");
        if (!cpu_max_file) {
            throw std::runtime_error("无法写入 cpu.max");
        }
        cpu_max_file << (static_cast<uint64_t>(limits.cpu_time_limit_s * 1000000)) << " 100000" << std::endl;
        cpu_max_file.close();

        // 设置内存限制
        std::ofstream(cgroup_path / "memory.max") << (limits.memory_limit_kb * 1024) << std::endl;
        std::ofstream(cgroup_path / "memory.swap.max") << "0" << std::endl;
        std::ofstream(cgroup_path / "memory.oom.group") << "1" << std::endl;  // 添加OOM控制
    }

    void CppActuator::monitorChild(pid_t pid,
                          bp::ipstream &stdout_stream,
                          bp::ipstream &stderr_stream,
                          const TimeStamp &start_time,
                          ExecutionResult &result,
                          const ResourceLimits &limits,
                          const fs::path& cgroup_path)
    {
        // 使用异步读取输出
        std::future<std::string> out_future = std::async(std::launch::async, [](bp::ipstream& stdout_stream) {
            std::ostringstream ss;
            char buffer[4096];
            while (stdout_stream) {
                stdout_stream.read(buffer, sizeof(buffer));
                std::streamsize n = stdout_stream.gcount();
                if (n > 0) {
                    ss.write(buffer, n);
                }
            }
            return ss.str();
        }, std::ref(stdout_stream));

        std::future<std::string> err_future = std::async(std::launch::async, [](bp::ipstream& stderr_stream) {
            std::ostringstream ss;
            char buffer[4096];
            while (stderr_stream) {
                stderr_stream.read(buffer, sizeof(buffer));
                std::streamsize n = stderr_stream.gcount();
                if (n > 0) {
                    ss.write(buffer, n);
                }
            }
            return ss.str();
        }, std::ref(stderr_stream));

      int status = 0;
        bool timed_out = false;
        const auto timeout_time = start_time + std::chrono::microseconds(
            static_cast<int64_t>(limits.wall_time_limit_s * 1000000));
        
        // ====== 修复5: 使用select避免忙等待 ======
        while (true) {
            // 检查超时
            const auto now = std::chrono::steady_clock::now();
            if (now >= timeout_time) {
                timed_out = true;
                kill(pid, SIGKILL);
                LOG_WARN("Process {} timed out, sent SIGKILL", pid);
                break;
            }
            
            // 使用select等待进程退出
            timeval tv{0, 10000}; // 10ms
            fd_set fds;
            FD_ZERO(&fds);
            
            int ret = select(0, nullptr, nullptr, nullptr, &tv);
            if (ret < 0 && errno != EINTR) {
                LOG_ERROR("select error: {}", strerror(errno));
                break;
            }
            
            // 检查子进程状态
            pid_t ret_pid = waitpid(pid, &status, WNOHANG);
            if (ret_pid == pid) {
                break;
            } else if (ret_pid == -1) {
                if (errno == ECHILD) {
                    LOG_DEBUG("Child process {} not found", pid);
                    break;
                }
                LOG_ERROR("waitpid error: {}", strerror(errno));
                break;
            }
            
            // 检查cgroup是否存在
            if (!fs::exists(cgroup_path)) {
                LOG_DEBUG("Cgroup {} disappeared", cgroup_path.string());
                break;
            }
        }

        // 如果超时后进程仍在运行，强制终止
        if (timed_out) {
            int attempts = 0;
            while (attempts < 5 && waitpid(pid, &status, WNOHANG) == 0) {
                kill(pid, SIGKILL);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                attempts++;
            }
        }

        // 确保回收子进程
        int wait_status;
        if (waitpid(pid, &wait_status, 0) < 0) {
            if (errno != ECHILD) {
                LOG_ERROR("Final waitpid failed: {}", strerror(errno));
            }
        }

        // 设置结果
        result.stdout = out_future.get();
        result.stderr = err_future.get();
        result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        result.signal = WIFSIGNALED(status) ? WTERMSIG(status) : 0;

        // 读取cgroup中的最大内存使用量
        int mem_peak = 0;
        std::ifstream mem_file(cgroup_path / "memory.peak");
        if (mem_file) {
            mem_file >> mem_peak;
            mem_peak /= 1024; // 转换为KB
        }
        result.memory_kb = mem_peak;

        result.create_at = start_time;
        result.finish_at = std::chrono::steady_clock::now();

        // 判断退出原因
        if (timed_out) {
            LOG_DEBUG("Process timed out.");
        } else if (result.memory_kb >= limits.memory_limit_kb) {
            LOG_DEBUG("Process exceeds memory limit: {} KB > {} KB.", result.memory_kb, limits.memory_limit_kb);
        } else if (result.signal == SIGSEGV && 
                result.memory_kb < limits.memory_limit_kb) {
            LOG_DEBUG("Process Segmentation Fault, but not exceed memory limit: {} KB < {} KB.", result.memory_kb, limits.memory_limit_kb);
        } else if (result.signal != 0 || result.exit_code != 0) {
            LOG_DEBUG("Process return error code {} or signal {}", result.exit_code, result.signal);
        } else {
            LOG_DEBUG("Normal exit, return code {}.", result.exit_code);
        }
    }

    void CppActuator::cleanupCgroups(const fs::path& cgroup_path)
    {
        try {
            // 确保cgroup目录存在
            if (!fs::exists(cgroup_path)) {
                std::cout << "Error: " << cgroup_path << " not found." << std::endl;
                return;
            }
            
            // 检查是否有进程残留
            auto procs_path = cgroup_path / "cgroup.procs";
            if (fs::exists(procs_path)) {
                std::ifstream procs_file(procs_path);
                if (procs_file.is_open()) {
                    std::string line;
                    while (std::getline(procs_file, line)) {
                        if (!line.empty()) {
                            std::cout << "Warning: process " << line << " still running in ";
                            return;
                        }
                    }
                }
            }
            
            // 递归删除子cgroup
            for (const auto& entry : fs::directory_iterator(cgroup_path)) {
                if (entry.is_directory()) {
                    cleanupCgroups(entry.path());
                }
            }
            
            // 删除当前cgroup目录
            int retry_count = 0;
            while (retry_count < 3) {
                try {
                    if (fs::remove(cgroup_path)) {
                        std::cout << "Cleaned up cgroup: " << cgroup_path.string() << std::endl;
                        return;
                    }
                } catch (const fs::filesystem_error& e) {
                    if (e.code() == std::errc::directory_not_empty) {
                        std::cout << "Cannot delete cgroup: " << cgroup_path.string() << std::endl;
                        std::cout << "Removing cgroup anyway..." << std::endl;
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        retry_count++;
                    } else {
                        throw;
                    }
                }
            }
            
            std::cerr << "Failed to remove cgroup: " << cgroup_path.string() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}