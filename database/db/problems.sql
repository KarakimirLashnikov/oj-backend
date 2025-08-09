-- 题目信息表
CREATE TABLE problems (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    title VARCHAR(127) NOT NULL UNIQUE,    -- 题目标题，用于查询
    difficulty ENUM('Beginner', 'Basic', 'Intermediate', 'Advanced', 'Expert') NOT NULL,
    description TEXT,                      -- 题目描述
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_title (title),               -- 索引加速查询
    INDEX idx_difficulty (difficulty)      -- 难易度索引
);
