CREATE DATABASE IF NOT EXISTS judgedb;
USE judgedb;
CREATE TABLE users (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_uuid VARCHAR(36) NOT NULL UNIQUE,
    username VARCHAR(64) NOT NULL UNIQUE,  -- 用户名
    email VARCHAR(128) NOT NULL,           -- 邮箱
    password_hash VARCHAR(128) NOT NULL,   -- 密码哈希
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_username (username),
    INDEX idx_user_uuid (user_uuid)
);
CREATE TABLE problems (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    title VARCHAR(128) NOT NULL UNIQUE,    -- 题目标题，用于查询
    difficulty ENUM('Beginner', 'Basic', 'Intermediate', 'Advanced', 'Expert') NOT NULL,
    description TEXT,                      -- 题目描述
    author_id BIGINT UNSIGNED,                  -- 作者
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (author_id) REFERENCES users(id) ON DELETE SET NULL,
    INDEX idx_title (title),               -- 索引加速查询
    INDEX idx_difficulty (difficulty)      -- 难易度索引
);
CREATE TABLE problem_limits (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    problem_id BIGINT UNSIGNED NOT NULL,
    language VARCHAR(16) NOT NULL,    -- 编程语言
    time_limit_ms INT UNSIGNED NOT NULL, -- 时间限制
    extra_time_ms INT UNSIGNED DEFAULT 500,
    wall_time_ms INT UNSIGNED NOT NULL,
    memory_limit_kb INT UNSIGNED NOT NULL,  -- 内存限制
    stack_limit_kb INT UNSIGNED NOT NULL,  -- 栈限制
    FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE,
    INDEX idx_problem_id (problem_id),    -- 索引加速查询
    UNIQUE (problem_id, language)
);
CREATE TABLE test_cases (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    problem_id BIGINT UNSIGNED NOT NULL,           -- 关联的题目ID
    stdin_data TEXT NOT NULL,                        -- 测试用例输入
    expected_output TEXT NOT NULL,              -- 期望输出
    sequence INT UNSIGNED NOT NULL,
    FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE,
    INDEX idx_problem_id (problem_id)           -- 索引加速查询
);
CREATE TABLE submissions (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    sub_uuid VARCHAR(36) NOT NULL UNIQUE,         -- 评测提交ID
    user_id BIGINT UNSIGNED NOT NULL,                 -- 用户ID
    problem_id BIGINT UNSIGNED NOT NULL,              -- 题目ID
    language VARCHAR(16) NOT NULL,                 -- 编程语言
    code TEXT NOT NULL,                            -- 源代码
    overall_status VARCHAR(32) NOT NULL,           -- 整体评测状态
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_problem_id (problem_id)
);
CREATE TABLE judge_results (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    submission_id BIGINT UNSIGNED NOT NULL,  -- 提交ID
    test_case_id BIGINT UNSIGNED NOT NULL,   -- 测试用例ID
    status VARCHAR(32) NOT NULL,          -- 测试用例状态
    cpu_time_ms INT UNSIGNED NOT NULL,    -- CPU时间(ms)
    memory_kb INT UNSIGNED NOT NULL,      -- 内存使用(KB)
    exit_code INT NOT NULL,               -- 退出代码
    signal_code INT NOT NULL,             -- 终止信号
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (submission_id) REFERENCES submissions(id) ON DELETE CASCADE,
    FOREIGN KEY (test_case_id) REFERENCES test_cases(id) ON DELETE CASCADE,
    INDEX idx_submission_id (submission_id),
    INDEX idx_status (status)
);