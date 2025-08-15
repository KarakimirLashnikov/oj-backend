-- 测试用例表
CREATE TABLE test_cases (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    problem_id BIGINT UNSIGNED NOT NULL,           -- 关联的题目ID
    stdin_data TEXT NOT NULL,                        -- 测试用例输入
    expected_output TEXT NOT NULL,              -- 期望输出
    sequence INT UNSIGNED NOT NULL,
    FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE,
    INDEX idx_problem_id (problem_id)           -- 索引加速查询
);