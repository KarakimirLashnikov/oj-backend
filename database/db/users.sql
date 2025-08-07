CREATE TABLE users (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE,  -- 用户名
    email VARCHAR(100) NOT NULL UNIQUE,    -- 邮箱
    password_hash VARCHAR(255) NOT NULL,   -- 密码哈希
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_username (username),
    INDEX idx_email (email)
);