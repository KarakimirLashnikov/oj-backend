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

namespace Judge
{
    void CppActuator::joinCgroup(const fs::path& cgroup_path) 
    {
        std::ofstream procs_file(cgroup_path / "cgroup.procs");
        if (!procs_file) {
            throw std::runtime_error("join cgroup.procs failed: " + 
                std::string(strerror(errno)));
        }
        procs_file << getpid() << std::endl;
        LOG_DEBUG("Process {} joined cgroup {}", getpid(), cgroup_path.string());
    }

    ExecutionResult CppActuator::execute(fs::path exe_path, const ResourceLimits &limits, std::string_view stdin_data)
    {
        LOG_DEBUG("Executing: {}", exe_path.string());
        
        // 确保可执行文件存在且有权限
        if (!fs::exists(exe_path)) {
            throw std::runtime_error("Executable not found: " + exe_path.string());
        }
        if (access(exe_path.c_str(), X_OK) != 0) {
            throw std::runtime_error("No execute permission: " + exe_path.string());
        }

        // 使用匿名管道
        int stdout_pipe[2], stderr_pipe[2], stdin_pipe[2];
        if (pipe(stdout_pipe)) throw std::runtime_error("pipe stdout failed");
        if (pipe(stderr_pipe)) throw std::runtime_error("pipe stderr failed");
        if (pipe(stdin_pipe)) throw std::runtime_error("pipe stdin failed");

        auto start_time = std::chrono::steady_clock::now();
        
        pid_t child_pid = fork();
        if (child_pid < 0) {
            throw std::system_error(std::error_code(errno, std::system_category()), "fork error");
        }
        else if (child_pid == 0) {
            // 子进程逻辑
            runChildProcess(stdin_pipe, stdout_pipe, stderr_pipe, exe_path, limits);
        }
        
        // 父进程逻辑
        close(stdin_pipe[0]);   // 关闭子端读
        close(stdout_pipe[1]);  // 关闭子端写
        close(stderr_pipe[1]);  // 关闭子端写
        
        // 写入输入数据
        if (!stdin_data.empty()) {
            ssize_t written = write(stdin_pipe[1], stdin_data.data(), stdin_data.size());
            if (written < 0) {
                LOG_ERROR("Failed to write stdin: {}", strerror(errno));
            }
        }
        close(stdin_pipe[1]);  // 关闭输入管道
        
        // 创建并设置cgroup
        fs::path cgroup_path = createCgroupForProcess(child_pid, limits);
        
        ExecutionResult result;
        this->monitorChild(child_pid, 
                          stdout_pipe[0], 
                          stderr_pipe[0], 
                          start_time, 
                          result, 
                          limits,
                          cgroup_path);

        // 关闭管道
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        return result;
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
        
        // 如果执行失败
        dprintf(STDERR_FILENO, "execv failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fs::path CppActuator::createCgroupForProcess(pid_t pid, const ResourceLimits& limits)
    {
        fs::path cgroup_root = "/sys/fs/cgroup";
        if (!fs::exists(cgroup_root)) {
            cgroup_root = "/sys/fs/cgroup/unified";
        }
        std::string cgroup_name = "judge_" + std::to_string(getpid()) + "_" + std::to_string(pid);
        fs::path cgroup_path = cgroup_root / cgroup_name;
        
        LOG_DEBUG("Creating cgroup at: {}", cgroup_path.string());
        
        // 创建cgroup目录
        if (!fs::create_directories(cgroup_path)) {
            throw std::runtime_error("Failed to create cgroup directory: " + cgroup_path.string());
        }
        
        // 设置cgroup限制
        setupCgroupLimits(limits, cgroup_path);
        
        // 将进程加入cgroup
        std::ofstream procs_file(cgroup_path / "cgroup.procs");
        if (!procs_file) {
            throw std::runtime_error("无法写入 cgroup.procs: " + 
                std::string(strerror(errno)) + " at " + 
                (cgroup_path / "cgroup.procs").string());
        }
        procs_file << pid << std::endl;
        procs_file.close();
        
        LOG_DEBUG("Cgroup setup completed for PID: {}", pid);
        return cgroup_path;
    }

    void CppActuator::setupCgroupLimits(const ResourceLimits &limits, const fs::path& cgroup_path)
    {
        LOG_DEBUG("Setting cgroup limits at: {}", cgroup_path.string());
        
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
        }
        
        // 4. 降低权限
        if (setgid(1000) != 0 || setuid(1000) != 0) {
            perror("Failed to drop privileges");
            exit(EXIT_FAILURE);
        }

        const char* policy = R"KAFEL(POLICY sample {
	ALLOW {
		write {
			(fd & buf) != 123
		}
	}
}

USE sample DEFAULT KILL)KAFEL";

        kafel_ctxt_t ctxt = kafel_ctxt_create();
        kafel_set_input_string(ctxt, policy);
        struct sock_fprog prog;
        int compile_result = kafel_compile(ctxt, &prog);
        if (compile_result != 0) {
            const char* error_msg = kafel_error_msg(ctxt);
            dprintf(STDERR_FILENO, "Kafel编译失败: %s\n", error_msg);
            kafel_ctxt_destroy(&ctxt);
            exit(EXIT_FAILURE);
        }
        kafel_ctxt_destroy(&ctxt);
        
        // 加载策略
        scmp_filter_ctx ctx{ seccomp_init(SCMP_ACT_KILL) };
        if (!ctx) {
            perror("faild to init seccomp filter");
            free(prog.filter);
            exit(EXIT_FAILURE);
        }
        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1) {
            perror("faild to load filter");
            seccomp_release(ctx);
            free(prog.filter);
            exit(EXIT_FAILURE);
        }
        free(prog.filter);
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
                if (errno == EINTR) continue;
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
            
            // 新增：读取CPU时间统计
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
        }
        
        LOG_DEBUG("Process exited with code: {}, signal: {}", 
                 result.exit_code, result.signal);
        LOG_DEBUG("STDOUT size: {}, STDERR size: {}", 
                 result.stdout.size(), result.stderr.size());
        
        result.create_at = start_time;
        result.finish_at = std::chrono::steady_clock::now();
        
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
        if (timeout) {
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

        auto str{ std::format("memeory: {}\nstderr: {}\nsignal: {}\nexit_code: {}\nstdout: {}\ncpu_time_us: {}us\n", result.memory_kb, result.stderr, result.signal, result.exit_code, result.stdout, result.cpu_time_us) };
        LOG_INFO("result:\n{}", str.c_str());

        // 清理cgroup - 不再使用线程延迟清理
        cleanupCgroup(cgroup_path);
    }

    void CppActuator::cleanupCgroup(const fs::path& cgroup_path)
    {
        try {
            if (!fs::exists(cgroup_path)) return;
            
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
                    attempts++;
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