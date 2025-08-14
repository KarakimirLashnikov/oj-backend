CREATE TABLE users (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_uuid VARCHAR(36) NOT NULL UNIQUE,
    username VARCHAR(64) NOT NULL UNIQUE,  -- 用户名
    email VARCHAR(128) NOT NULL,           -- 邮箱
    password_hash VARCHAR(128) NOT NULL,   -- 密码哈希
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_username (username),
    INDEX idx_user_uuid (user_uuid)
);