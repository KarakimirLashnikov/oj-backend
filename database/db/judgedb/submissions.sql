CREATE TABLE submissions (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    submission_id VARCHAR(37) NOT NULL UNIQUE, -- 评测提交ID
    user_id INT UNSIGNED NOT NULL,                      -- 用户ID
    problem_id INT UNSIGNED NOT NULL,                   -- 题目ID
    language VARCHAR(15) NOT NULL,             -- 编程语言
    code TEXT NOT NULL,                        -- 源代码
    overall_status VARCHAR(25) NOT NULL,       -- 整体评测状态
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_problem_id (problem_id)
);