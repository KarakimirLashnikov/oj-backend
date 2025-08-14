-- 题目信息表
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
