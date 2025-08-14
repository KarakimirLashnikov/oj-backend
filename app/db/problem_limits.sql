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