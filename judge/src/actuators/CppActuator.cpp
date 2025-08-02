#include "actuators/CppActuator.hpp"
#include "utilities/ExecutionResult.hpp"
#include <seccomp.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mount.h>

namespace Judge
{
    ExecutionResult CppActuator::execute(fs::path exe_path, const ResourceLimits &limits, std::string_view stdin_data)
    {
        // 输出重定向
        bp::pipe stdout_pipe, stderr_pipe, stdin_pipe;
        bp::ipstream stdout_stream, stderr_stream;

        // 准备子进程环境
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
            // 关闭不需要的管道端
            stdin_pipe.close(); // 关闭父进程的读取端
            stdout_pipe.close();  // 关闭父进程的写入端
            stderr_pipe.close();
            
            // 创建唯一的cgroup路径
            fs::path cgroup_path = fs::path("/sys/fs/cgroup") / ("judge_" + std::to_string(child_pid));
            
            // 设置cgroup
            this->setupCgroups(child_pid, limits, cgroup_path);

            // 写入输入数据
            if (!stdin_data.empty()) {
                bp::opstream in{ stdin_pipe };
                in.write(stdin_data.data(), stdin_data.size());
                in.flush();
                in.close();
            } else {
                stdin_pipe.close();
            }

            ExecutionResult result;
            this->monitorChild(child_pid, stdout_stream, stderr_stream, start_time, result, limits, cgroup_path);

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
        
        struct rlimit cpu_limit{
            .rlim_cur = static_cast<rlim_t>((limits.cpu_time_limit_s + limits.cpu_extra_time_s) * 1000000),
            .rlim_max = static_cast<rlim_t>((limits.cpu_time_limit_s + limits.cpu_extra_time_s) * 1000000)};
        setrlimit(RLIMIT_CPU, &cpu_limit);
        
        // 内存限制由cgroup控制，这里不设置RLIMIT_AS

        // Seccomp 白名单
        scmp_filter_ctx filter_ctx = seccomp_init(SCMP_ACT_KILL);
        if (!filter_ctx) {
            perror("seccomp_init failed");
            exit(EXIT_FAILURE);
        }
        this->allowSyscalls(filter_ctx);
        if (seccomp_load(filter_ctx) != 0) {
            perror("seccomp_load failed");
            exit(EXIT_FAILURE);
        }
        seccomp_release(filter_ctx);

        // 创建隔离命名空间
        if (unshare(CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWNET) != 0) {
            perror("unshare failed");
            exit(EXIT_FAILURE);
        }

        // 挂载proc文件系统
        if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, nullptr) != 0) {
            perror("mount proc failed");
        }

        // 限定用户权限
        if (setuid(1000) != 0) {
            perror("setuid failed");
            exit(EXIT_FAILURE);
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
            SCMP_SYS(clock_gettime), SCMP_SYS(getrandom)
        };

        for (int syscall : allowed_syscalls) {
            if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, syscall, 0) != 0) {
                fprintf(stderr, "Failed to add rule for syscall %d\n", syscall);
            }
        }

        // 特殊规则：限制文件描述符
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 1, SCMP_A0(SCMP_CMP_LE, 2));
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 1, SCMP_A0(SCMP_CMP_LE, 2));
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 1, SCMP_A0(SCMP_CMP_LE, 2));
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

        // 关闭所有管道描述符
        stdin_pipe.close();
        stdout_pipe.close();
        stderr_pipe.close();

        // 关闭所有可能的打开文件描述符
        for (int fd = sysconf(_SC_OPEN_MAX); fd > 2; fd--) {
            close(fd);
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
            throw std::runtime_error("Failed to create cgroup directory");
        }

        // 将进程加入cgroup
        std::ofstream(cgroup_path / "cgroup.procs") << pid << std::endl;

        // 设置CPU时间限制（单位：微秒）
        std::ofstream(cgroup_path / "cpu.max") 
            << (static_cast<uint64_t>(limits.cpu_time_limit_s * 1000000)) << " 100000" << std::endl;

        // 设置内存限制
        std::ofstream(cgroup_path / "memory.max") << (limits.memory_limit_kb * 1024) << std::endl;
        std::ofstream(cgroup_path / "memory.swap.max") << "0" << std::endl;
    }

    void CppActuator::monitorChild(pid_t pid,
                                   bp::ipstream &stdout_stream,
                                   bp::ipstream &stderr_stream,
                                   const TimeStamp &start_time,
                                   ExecutionResult &result,
                                   const ResourceLimits &limits,
                                   const fs::path& cgroup_path)
    {
        // 原子标志用于控制读取线程
        std::atomic<bool> child_running{true};
        
        // 读取输出和错误
        std::string output, error;
        std::thread out_thread([&] {
            char buffer[4096];
            while (child_running) {
                if (stdout_stream.read(buffer, sizeof(buffer))) {
                    output.append(buffer, stdout_stream.gcount());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            // 读取剩余数据
            while (stdout_stream.read(buffer, sizeof(buffer))) {
                output.append(buffer, stdout_stream.gcount());
            }
        });

        std::thread err_thread([&] {
            char buffer[4096];
            while (child_running) {
                if (stderr_stream.read(buffer, sizeof(buffer))) {
                    error.append(buffer, stderr_stream.gcount());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            while (stderr_stream.read(buffer, sizeof(buffer))) {
                error.append(buffer, stderr_stream.gcount());
            }
        });

        // 计算超时时间
        auto timeout_time = start_time + std::chrono::microseconds(
            static_cast<int64_t>(limits.wall_time_limit_s * 1000000));

        int status = 0;
        bool timed_out = false;
        bool process_exited = false;
        
        // 等待子进程退出或超时
        while (!process_exited) {
            // 检查是否超时
            auto current_time = std::chrono::steady_clock::now();
            if (current_time >= timeout_time) {
                timed_out = true;
                kill(pid, SIGKILL);
                break;
            }

            // 非阻塞等待子进程
            pid_t ret = waitpid(pid, &status, WNOHANG);
            if (ret == pid) {
                process_exited = true;
                break;
            } else if (ret == -1) {
                // 等待出错
                break;
            }

            // 检查cgroup是否已终止
            if (!fs::exists(cgroup_path / "cgroup.procs")) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // 标记子进程已结束
        child_running = false;
        
        // 如果超时后进程仍在运行，再次尝试终止
        if (timed_out) {
            int kill_attempts = 0;
            while (kill_attempts < 5 && waitpid(pid, &status, WNOHANG) == 0) {
                kill(pid, SIGKILL);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                kill_attempts++;
            }
        }

        // 确保回收子进程
        if (!process_exited && waitpid(pid, &status, 0) < 0) {
            perror("waitpid failed");
        }

        // 等待输出线程结束
        out_thread.join();
        err_thread.join();

        // 从cgroup读取内存使用峰值
        uint64_t mem_peak = 0;
        std::ifstream mem_file(cgroup_path / "memory.peak");
        if (mem_file) {
            mem_file >> mem_peak;
            mem_peak /= 1024; // 转换为KB
        }

        // 读取CPU时间使用量
        uint64_t cpu_usage = 0;
        std::ifstream cpu_file(cgroup_path / "cpu.stat");
        if (cpu_file) {
            std::string line;
            while (std::getline(cpu_file, line)) {
                if (line.find("usage_usec") != std::string::npos) {
                    sscanf(line.c_str(), "usage_usec %lu", &cpu_usage);
                    break;
                }
            }
        }

        // 设置结果
        result.stdout = std::move(output);
        result.stderr = std::move(error);
        result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        result.signal = WIFSIGNALED(status) ? WTERMSIG(status) : 0;
        result.memory_kb = static_cast<int>(mem_peak);
        result.create_at = start_time;
        result.finish_at = std::chrono::steady_clock::now();

        // 判断退出原因
        if (timed_out) {
            result.status = TestStatus::TIME_LIMIT_EXCEEDED;
        } else if (result.memory_kb >= limits.memory_limit_kb) {
            result.status = TestStatus::MEMORY_LIMIT_EXCEEDED;
        } else if (result.signal == SIGSEGV && 
                result.memory_kb < limits.memory_limit_kb) {
            result.status = TestStatus::RUNTIME_ERROR;
        } else if (result.signal != 0 || result.exit_code != 0) {
            result.status = TestStatus::RUNTIME_ERROR;
        } else {
            result.status = TestStatus::UNKNOWN;
        }

        // 清理cgroup
        fs::remove_all(cgroup_path);
    }
}