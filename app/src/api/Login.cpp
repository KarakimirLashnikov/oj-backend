#include <nlohmann/json.hpp>
#include <sodium.h>
#include "api/Login.hpp"
#include "Types.hpp"
#include "Application.hpp"
#include "judgedb/UserInquirer.hpp"
#include "judgedb/UserWriter.hpp"
#include "ParameterException.hpp"
#include "SystemException.hpp"

namespace OJApp::Login
{
    using Core::Http::InternalServerError;
    using Core::Http::BadRequest;
    using Core::Http::OK;
    using Exceptions::ParameterException::ExceptionType::MISSING_PARAM;
    using Exceptions::ParameterException::ExceptionType::VALUE_ERROR;

    void login(const httplib::Request &req, httplib::Response &res) {
        try {
            // 解析 JSON 请求体
            auto json = njson::parse(req.body);
            std::string username{}, password{};
            if (json.contains("username")) {
                username = json.at("username").get<std::string>();
            } else {
                throw Exceptions::ParameterException(MISSING_PARAM, "username", "username 字段缺失");
            }

            if (json.contains("password")) {
                password = json.at("password").get<std::string>();
            } else {
                throw Exceptions::ParameterException(MISSING_PARAM, "password", "password 字段缺失");
            }

            // 1. 从数据库获取用户记录
            Core::Configurator& cfg{ App.getConfigurator() };
            JudgeDB::UserInquirer userIq{
                cfg.get<std::string>("judgedb", "HOST", "127.0.0.1"),
                cfg.get<std::string>("judgedb", "USERNAME", "root"),
                cfg.get<std::string>("judgedb", "PASSWORD", ""),
                cfg.get<std::string>("judgedb", "DATABASE", "userdb")
            };
            if (!userIq.isExist(username)) {
                throw Exceptions::ParameterException(VALUE_ERROR, "username", "用户名不存在");
            }
            
            // 2. 验证密码
            std::string hash_str{ userIq.getPassword(username) };
            if (crypto_pwhash_str_verify(
                    hash_str.c_str(),
                    password.c_str(),
                    password.length()
            ) != 0) {
                throw Exceptions::ParameterException(VALUE_ERROR, "password", "密码错误");
            }
            
            // 成功响应 (实际应返回会话令牌)
            res.status = OK;
            njson response{ njson{{"status", "success"}, {"message", "登陆成功"}}};
            res.set_content(response.dump(), "application/json");
        } catch (const Exceptions::ParameterException& e) {
            res.status = BadRequest;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        } catch (const sql::SQLException& e) {
            res.status = InternalServerError;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = InternalServerError;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        }
    }

    void signup(const httplib::Request &req, httplib::Response &res) {
        try {
            // 解析 JSON 请求体
            auto json{ njson::parse(req.body) };
            std::string username{}, password{}, email{};
            if (json.contains("username")) {
                username = json.at("username").get<std::string>();
            } else {
                throw Exceptions::ParameterException(MISSING_PARAM, "username", "username 字段缺失");
            }
            
            if (json.contains("password")) {
                password = json.at("password").get<std::string>();
            } else {
                throw Exceptions::ParameterException(MISSING_PARAM, "password", "<PASSWORD>");
            }
            
            if (json.contains("email")) {
                email = json.at("email").get<std::string>();
            } else {
                throw Exceptions::ParameterException(MISSING_PARAM, "email", "email 字段缺失");
            }

            // 生成随机盐值
            unsigned char salt[crypto_pwhash_SALTBYTES];
            randombytes_buf(salt, sizeof(salt));

            // 生成密码哈希 (使用 Argon2id 算法)
            unsigned char hash[crypto_pwhash_STRBYTES];
            if (crypto_pwhash_str(
                reinterpret_cast<char*>(hash),
                password.c_str(),
                password.length(),
                crypto_pwhash_OPSLIMIT_INTERACTIVE,
                crypto_pwhash_MEMLIMIT_INTERACTIVE
            ) != 0) {
                throw Exceptions::makeSystemException("crypto_pwhash_str failed", __FILE__, __LINE__);
            }

            // 1. 检查用户名是否已存在
            Core::Configurator& cfg{ App.getConfigurator() };
            JudgeDB::UserInquirer userIq{
                cfg.get<std::string>("judgedb", "HOST", "127.0.0.1"),
                cfg.get<std::string>("judgedb", "USERNAME", "root"),
                cfg.get<std::string>("judgedb", "PASSWORD", ""),
                cfg.get<std::string>("judgedb", "DATABASE", "userdb")
            };
            if (userIq.isExist(username)) {
                throw Exceptions::ParameterException(VALUE_ERROR, "username", "用户名已存在");
            }
            
            // 2. 存储到数据库
            JudgeDB::UserWriter userWt{
                cfg.get<std::string>("judgedb", "HOST", "127.0.0.1"),
                cfg.get<std::string>("judgedb", "USERNAME", "root"),
                cfg.get<std::string>("judgedb", "PASSWORD", ""),
                cfg.get<std::string>("judgedb", "DATABASE", "userdb")
            };
            userWt.createUser(username, email, reinterpret_cast<const char*>(hash));
            
            // 成功响应
            res.status = OK;
            njson response{ njson{{"status", "success"}, {"message", "注册成功"}}};
            res.set_content(response.dump(), "application/json");
        } catch (const Exceptions::ParameterException& e) {
            res.status = BadRequest;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        } catch (const sql::SQLException& e) {
            res.status = InternalServerError;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = InternalServerError;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        }
    }
}

