CREATE TABLE users (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_uuid VARCHAR(36) NOT NULL UNIQUE, -- 用户id
    username VARCHAR(64) NOT NULL UNIQUE,  -- 用户名
    email VARCHAR(128) NOT NULL UNIQUE,    -- 邮箱
    password_hash VARCHAR(128) NOT NULL,   -- 密码哈希
    salt VARCHAR(64) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_username (username),
    INDEX idx_email (email)
);