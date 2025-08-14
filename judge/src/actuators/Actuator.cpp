#include <kafel.h>
#include <seccomp.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <asm/unistd.h>
#include <poll.h>
#include "Logger.hpp"
#include "actuators/Actuator.hpp"
#include "utilities/ExecutionResult.hpp"
#include "FileException.hpp"
#include "SystemException.hpp"

static std::once_flag ONCE_FLAG{};
static sock_fprog SHARED_SECCOMP{};
static bool IS_SYSTEM_INITIALIZED{false};
static constexpr const char *CGROUP_ROOT{"/sys/fs/cgroup"};
static constexpr int FATHER_PIPE_INDEX{0};
static constexpr int CHILD_PIPE_INDEX{1};

namespace Judge
{
    Core::Configurator* Actuator::s_ConfiguratorPtr{nullptr};

    void Actuator::initSystem(Core::Configurator &cfg)
    {
        std::call_once(::ONCE_FLAG, [&]() {  
            // 1. 检查/proc目录
            Exceptions::checkFileExists("/proc");
            Exceptions::checkFileWritable("/proc");
            
            // 2. 检查cgroup根目录
            Exceptions::checkFileExists(::CGROUP_ROOT);
            Exceptions::checkFileWritable(::CGROUP_ROOT);

            fs::path kafel_file_path{ cfg.get<std::string>("kafel", "KAFEL_POLICY_FILE", "./kafel.policy") };
            Exceptions::checkFileExists(kafel_file_path);
            Exceptions::checkFileReadable(kafel_file_path);
            std::ifstream f{ kafel_file_path };
            if (!f) {
                throw Exceptions::makeSystemException("kafel policy file " + kafel_file_path.string() + " open failed");
            }
            std::string kafel_policy{ std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>() };

            kafel_ctxt_t ctxt = kafel_ctxt_create();
            kafel_set_input_string(ctxt, kafel_policy.c_str());
            
            int compile_result = kafel_compile(ctxt, &::SHARED_SECCOMP);
            if (compile_result != 0) {
                // 安全地获取错误信息
                const char* error_ptr = kafel_error_msg(ctxt);
                std::string error_msg = error_ptr ? error_ptr : "unknown error";
                
                // 记录原始错误信息
                LOG_ERROR("Kafel compile failed: {}", error_msg);
                
                // 转换为十六进制以便诊断
                std::string hex_dump;
                for (const char* p = error_ptr; *p; ++p) {
                    char buf[5];
                    snprintf(buf, sizeof(buf), "%02X ", static_cast<unsigned char>(*p));
                    hex_dump += buf;
                }
                LOG_ERROR("error info(hex): {}", hex_dump);
                
                kafel_ctxt_destroy(&ctxt);
                throw Exceptions::SystemException(compile_result, "Kafel compile failed, error_msg: " + error_msg);
            }
            kafel_ctxt_destroy(&ctxt);
            
            // 4. 注册退出时释放资源的函数
            atexit([]() {
                if (::SHARED_SECCOMP.filter) {
                    free(::SHARED_SECCOMP.filter);
                    ::SHARED_SECCOMP.filter = nullptr;
                }
            });
            
            Actuator::s_ConfiguratorPtr = &cfg;
            ::IS_SYSTEM_INITIALIZED = true;
            LOG_INFO("Global initialization completed"); 
        });
    }

    ExecutionResult Actuator::execute(const fs::path &exe_path, const LimitsInfo &limits, std::string_view stdin_data)
    {
        // 准备管道
        int stdin_pipe[2]{}, stdout_pipe[2]{}, stderr_pipe[2]{};
        this->createPipes(stdin_pipe, stdout_pipe, stderr_pipe);
        LOG_INFO("Pipes created");

        auto start_at{std::chrono::steady_clock::now()};

        // 开启子线程
        pid_t child_pid{fork()};
        LOG_INFO("Child created, pid: {}", child_pid);

        if (child_pid < 0)
        {
            throw Exceptions::makeSystemException(std::string("fork error: ") + __FILE__ + ":" + std::to_string(__LINE__));
        }
        else if (child_pid == 0)
        {
            // 子进程
            this->setChildEnv(stdin_pipe, stdout_pipe, stderr_pipe, exe_path, limits);
            this->runChildProcess(exe_path);
            // 异常退出
            _exit(EXIT_FAILURE);
        }

        // 父进程
        // 关闭不需要的端口
        close(stdin_pipe[FATHER_PIPE_INDEX]);
        close(stdout_pipe[CHILD_PIPE_INDEX]);
        close(stderr_pipe[CHILD_PIPE_INDEX]);

        // 写入输入数据
        if (!stdin_data.empty())
        {
            size_t total_written = 0;
            while (total_written < stdin_data.size())
            {
                ssize_t written = write(stdin_pipe[1], stdin_data.data() + total_written, stdin_data.size() - total_written);
                if (written < 0)
                {
                    // 释放资源，准备退出
                    kill(child_pid, SIGKILL);
                    waitpid(child_pid, nullptr, 0);
                    close(stdin_pipe[CHILD_PIPE_INDEX]);
                    close(stdout_pipe[FATHER_PIPE_INDEX]);
                    close(stderr_pipe[FATHER_PIPE_INDEX]);
                    throw Exceptions::makeSystemException(std::string("Failed to write data to stdin pipe: ") + strerror(errno));
                }
                total_written += written;
            }
        }
        // 写完关闭子进程的写端
        close(stdin_pipe[CHILD_PIPE_INDEX]);

        ExecutionResult result{};
        fs::path cgroup_path{};
        // 创建 cgroup
        try
        {
            cgroup_path = this->createCgroupForProcess(child_pid, limits);
            LOG_INFO("Cgroup setup completed for PID: {}", child_pid);

            // 监控子进程
            LOG_INFO("Monitoring PID: {}", child_pid);
            monitorChild(child_pid, stdout_pipe[FATHER_PIPE_INDEX], stderr_pipe[FATHER_PIPE_INDEX], start_at, result, limits, cgroup_path);
            LOG_INFO("Monitoring finished");
        }
        catch (const std::exception &e)
        {
            // 清理资源
            close(stdout_pipe[FATHER_PIPE_INDEX]);
            close(stderr_pipe[FATHER_PIPE_INDEX]);
            cleanupCgroup(cgroup_path);
            LOG_ERROR("unexpected error occured: {}", e.what());
            throw Exceptions::makeSystemException(e.what());
        }

        close(stdout_pipe[FATHER_PIPE_INDEX]);
        close(stderr_pipe[FATHER_PIPE_INDEX]);
        cleanupCgroup(cgroup_path);
        LOG_INFO("Cgroup cleaned for PID: {}", child_pid);
        return result;
    }

    void Actuator::setupLanguageEnv()
    {}

    void Actuator::createPipes(int stdin_pipe[2], int stdout_pipe[2], int stderr_pipe[2])
    {
        if (pipe(stdin_pipe) == -1)
            throw Exceptions::makeSystemException("Failed to create stdin pipe: " + std::string(strerror(errno)));
        if (pipe(stdout_pipe) == -1)
        {
            close(stdin_pipe[0]);
            close(stdin_pipe[1]);
            throw Exceptions::makeSystemException("Failed to create stdout pipe: " + std::string(strerror(errno)));
        }
        if (pipe(stderr_pipe) == -1)
        {
            close(stdin_pipe[0]);
            close(stdin_pipe[1]);
            close(stdout_pipe[0]);
            close(stdout_pipe[1]);
            throw Exceptions::makeSystemException("Failed to create stderr pipe: " + std::string(strerror(errno)));
        }

        // 设置非阻塞模式
        if (fcntl(stdout_pipe[FATHER_PIPE_INDEX], F_SETFL, O_NONBLOCK) == -1)
        {
            close(stdin_pipe[0]);
            close(stdin_pipe[1]);
            close(stdout_pipe[0]);
            close(stdout_pipe[1]);
            close(stderr_pipe[0]);
            close(stderr_pipe[1]);
            throw Exceptions::makeSystemException("fcntl error: " + std::to_string(errno));
        }
        if (fcntl(stderr_pipe[FATHER_PIPE_INDEX], F_SETFL, O_NONBLOCK) == -1)
        {
            close(stdin_pipe[0]);
            close(stdin_pipe[1]);
            close(stdout_pipe[0]);
            close(stdout_pipe[1]);
            close(stderr_pipe[0]);
            close(stderr_pipe[1]);
            throw Exceptions::makeSystemException("fcntl error: " + std::to_string(errno));
        }
    }

    void Actuator::setChildEnv(int stdin_pipe[2]
                            , int stdout_pipe[2]
                            , int stderr_pipe[2]
                            , const fs::path& exe_path
                            , const LimitsInfo& limits)
    {
        close(stdin_pipe[CHILD_PIPE_INDEX]);
        close(stdout_pipe[FATHER_PIPE_INDEX]);
        close(stderr_pipe[FATHER_PIPE_INDEX]);
        
        // 重定向标准IO
        dup2(stdin_pipe[FATHER_PIPE_INDEX], STDIN_FILENO);
        dup2(stdout_pipe[CHILD_PIPE_INDEX], STDOUT_FILENO);
        dup2(stderr_pipe[CHILD_PIPE_INDEX], STDERR_FILENO);
        
        // 关闭多余描述符
        close(stdin_pipe[FATHER_PIPE_INDEX]);
        close(stdout_pipe[CHILD_PIPE_INDEX]);
        close(stderr_pipe[CHILD_PIPE_INDEX]);
        
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

        this->setupLanguageEnv();

        // 5. 降低权限
        if (setgid(1000) != 0 || setuid(1000) != 0) {
            perror("Failed to drop privileges");
            exit(EXIT_FAILURE);
        }
    }

    fs::path Actuator::createCgroupForProcess(pid_t pid, const LimitsInfo &limits)
    {
        fs::path cgroup_root = ::CGROUP_ROOT;
        std::string cgroup_name = "judge_" + std::to_string(getpid()) + "_" + std::to_string(pid);
        fs::path cgroup_path = cgroup_root / cgroup_name;

        // 创建cgroup目录
        if (!fs::create_directories(cgroup_path))
        {
            throw Exceptions::makeFileException(cgroup_path.c_str(), "create cgroup dir for child process");
        }

        // 设置cgroup限制
        setupCgroupLimits(limits, cgroup_path);

        // 将进程加入cgroup
        std::ofstream procs_file(cgroup_path / "cgroup.procs");
        if (!procs_file)
        {
            throw Exceptions::makeFileException(cgroup_path / "cgroup.procs", "open cgroup.procs");
        }
        procs_file << pid << std::endl;
        procs_file.close();

        return cgroup_path;
    }

    void Actuator::setupCgroupLimits(const LimitsInfo &limits, const fs::path &cgroup_path)
    {
        // 设置CPU时间限制
        std::ofstream cpu_max_file(cgroup_path / "cpu.max");
        if (cpu_max_file)
        {
            cpu_max_file << (static_cast<uint64_t>((limits.time_limit_s + limits.extra_time_s) * 1000000)) << " 1000000" << std::endl;
        }
        else
        {
            throw Exceptions::makeFileException(cgroup_path / "cpu.max", "open cpu.max");
        }

        // 设置内存限制
        std::ofstream(cgroup_path / "memory.max") << ((limits.memory_limit_kb + 100) * 1024) << std::endl;
        std::ofstream(cgroup_path / "memory.swap.max") << "0" << std::endl;
        std::ofstream(cgroup_path / "memory.oom.group") << "1" << std::endl;
    }

    void Actuator::monitorChild(pid_t pid,
                                int stdout_fd,
                                int stderr_fd,
                                const TimeStamp &start_time,
                                ExecutionResult &result,
                                const LimitsInfo &limits,
                                const fs::path &cgroup_path)
    {
        // 使用poll同时监控进程退出和输出
        struct pollfd fds[3] = {
            {.fd = stdout_fd, .events = POLLIN},
            {.fd = stderr_fd, .events = POLLIN},
            {.fd = -1, .events = 0}};

        std::string stdout_data{}, stderr_data{};
        char buffer[4096]{};
        bool process_exited{false};
        int status{0};

        const auto timeout_time = start_time + std::chrono::microseconds(
                                                   static_cast<int64_t>(limits.wall_time_s * 1000000));
        bool timeout{false};

        while (!process_exited)
        {
            // 检查超时
            const auto now{std::chrono::steady_clock::now()};
            if (now >= timeout_time)
            {
                LOG_WARN("Process {} timed out, sending SIGKILL", pid);
                kill(pid, SIGKILL);
                timeout = true;
                break;
            }

            // 计算剩余时间（毫秒）
            int timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeout_time - now).count();
            timeout_ms = std::max(10, std::min(100, timeout_ms)); // 限制在10-100ms

            int ret = poll(fds, 2, timeout_ms);
            if (ret < 0)
            {
                if (errno == EINTR)
                    continue; // 被信号打断，忽略
                LOG_ERROR("poll error: {}", strerror(errno));
                break;
            }

            // 处理stdout读取
            if (fds[0].revents & POLLIN) {
                ssize_t n = read(stdout_fd, buffer, sizeof(buffer));
                if (n > 0) {
                    stdout_data.append(buffer, n);
                } else if (n == 0) {
                    // 读到EOF，管道已关闭，标记不再监控
                    fds[0].fd = -1;
                } else { // n < 0
                    if (errno != EAGAIN) {
                        // 非EAGAIN错误（如EBADF），标记关闭
                        fds[0].fd = -1;
                    }
                    // EAGAIN：暂时无数据，继续监控
                }
            }

            // 处理stderr读取
            if (fds[1].revents & POLLIN) {
                ssize_t n = read(stderr_fd, buffer, sizeof(buffer));
                if (n > 0) {
                    stderr_data.append(buffer, n);
                } else if (n == 0) {
                    fds[1].fd = -1;
                } else {
                    if (errno != EAGAIN) {
                        fds[1].fd = -1;
                    }
                }
            }

            // 检查进程状态
            pid_t wpid = waitpid(pid, &status, WNOHANG);
            if (wpid == pid)
            {
                process_exited = true;
                LOG_DEBUG("Child process exited: PID={}", pid);
            }
            else if (wpid == -1)
            {
                if (errno == ECHILD)
                {
                    process_exited = true;
                    LOG_DEBUG("Child process not found, assuming exited");
                }
                else
                {
                    LOG_ERROR("waitpid error: {}", strerror(errno));
                }
            }

            // 检查是否所有输出都已关闭
            if (fds[0].fd == -1 && fds[1].fd == -1)
            {
                LOG_DEBUG("All output pipes closed");
                break;
            }
        }

        // 确保回收子进程
        if (!process_exited)
        {
            LOG_INFO("Waiting for child process to exit...");
            waitpid(pid, &status, 0);
        }

        // 读取剩余stdout数据
        while (true) {
            ssize_t n = read(stdout_fd, buffer, sizeof(buffer));
            if (n > 0) {
                stdout_data.append(buffer, n);
            } else if (n == 0) {
                // 读到EOF，退出循环
                break;
            } else { // n < 0
                if (errno == EAGAIN) {
                    // 暂时无数据，继续尝试读取
                    continue;
                } else {
                    // 其他错误（如管道已关闭），退出
                    break;
                }
            }
        }

        // 读取剩余stderr数据
        while (true) {
            ssize_t n = read(stderr_fd, buffer, sizeof(buffer));
            if (n > 0) {
                stderr_data.append(buffer, n);
            } else if (n == 0) {
                break;
            } else {
                if (errno == EAGAIN) {
                    continue;
                } else {
                    break;
                }
            }
        }


        result.stdout = std::move(stdout_data);
        result.stderr = std::move(stderr_data);
        result.test_result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        result.test_result.signal = WIFSIGNALED(status) ? WTERMSIG(status) : 0;

        try
        {
            // 读取内存峰值
            if (fs::exists(cgroup_path / "memory.peak"))
            {
                std::ifstream mem_file(cgroup_path / "memory.peak");
                if (mem_file)
                {
                    uint64_t mem_peak = 0;
                    mem_file >> mem_peak;
                    result.test_result.memory_kb = mem_peak / 1024;
                }
            }

            // 读取CPU时间统计
            if (fs::exists(cgroup_path / "cpu.stat"))
            {
                std::ifstream cpu_stat(cgroup_path / "cpu.stat");
                std::string line;
                while (std::getline(cpu_stat, line))
                {
                    if (line.find("usage_usec") == 0)
                    {
                        uint64_t cpu_us;
                        if (sscanf(line.c_str(), "usage_usec %lu", &cpu_us) == 1)
                        {
                            result.test_result.duration_us = cpu_us;
                        }
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Failed to read cgroup stats: {}", e.what());
            result.test_result.duration_us = 0;
            result.test_result.memory_kb = 0;
            result.test_result.status = TestStatus::INTERNAL_ERROR;
            throw Exceptions::makeSystemException("Failed to read cgroup statistics");
        }

        // 判断退出原因
        if (timeout)
        {
            result.test_result.status = TestStatus::TIME_LIMIT_EXCEEDED;
        }
        else if (result.test_result.memory_kb >= limits.memory_limit_kb)
        {
            result.test_result.status = TestStatus::MEMORY_LIMIT_EXCEEDED;
        }
        else if (result.test_result.signal != 0 || result.test_result.exit_code != 0)
        {
            result.test_result.status = TestStatus::RUNTIME_ERROR;
        }
        else
        {
            result.test_result.status = TestStatus::UNKNOWN; // 未判定 , 默认
        }
    }

    void Actuator::cleanupCgroup(const fs::path& cgroup_path)
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
            throw Exceptions::makeSystemException("Failed to remove cgroup after 3 attempts: " + cgroup_path.string());
        } catch (const std::exception& e) {
            LOG_ERROR("Cgroup cleanup error: {}", e.what());
            throw Exceptions::makeSystemException("Cgroup cleanup error: " + std::string(e.what()));
        }
    }
}