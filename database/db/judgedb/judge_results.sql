CREATE TABLE judge_results (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    submission_id INT UNSIGNED NOT NULL,  -- 提交ID
    test_case_id INT UNSIGNED NOT NULL,   -- 测试用例ID
    status VARCHAR(50) NOT NULL,          -- 测试用例状态
    cpu_time_us BIGINT UNSIGNED NOT NULL, -- CPU时间(微秒)
    memory_kb INT UNSIGNED NOT NULL,      -- 内存使用(KB)
    exit_code INT NOT NULL,               -- 退出代码
    signal INT NOT NULL,                  -- 终止信号
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (submission_id) REFERENCES submissions(id) ON DELETE CASCADE,
    FOREIGN KEY (test_case_id) REFERENCES test_cases(id) ON DELETE CASCADE,
    INDEX idx_submission_id (submission_id),
    INDEX idx_status (status)
);