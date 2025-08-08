-- 题目信息表
CREATE TABLE problems (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    title VARCHAR(128) NOT NULL UNIQUE,    -- 题目标题，用于查询
    time_limit_ms INT UNSIGNED NOT NULL,   -- 时间限制(毫秒)
    extra_time_ms INT UNSIGNED DEFAULT 500,-- 额外超时时间(毫秒)
    wall_time_ms INT UNSIGNED NOT NULL,    -- 运行时限(毫秒)
    memory_limit_kb INT UNSIGNED NOT NULL, -- 内存限制(KB)
    stack_limit_kb INT UNSIGNED,           -- 栈限制(KB)
    difficulty ENUM('Beginner', 'Basic', 'Intermediate', 'Advanced', 'Expert') NOT NULL,        -- 难易度(1-5，1最简单，5最难)
    description TEXT,                      -- 题目描述
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_title (title),               -- 索引加速查询
    INDEX idx_difficulty (difficulty)      -- 难易度索引
);
