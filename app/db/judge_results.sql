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