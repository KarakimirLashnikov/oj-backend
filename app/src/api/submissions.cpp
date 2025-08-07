#include <nlohmann/json.hpp>
#include "Application.hpp"
#include "api/submissions.hpp"
#include "utilities/Submission.hpp"
#include "utilities/Language.hpp"
#include "Types.hpp"
#include "Http.hpp"
#include "ParameterException.hpp"
#include "judgedb/UserInquirer.hpp"
#include "judgedb/ProblemInquirer.hpp"
#include "SystemException.hpp"

namespace OJApp::Submissions
{
    using Judge::Submission;
    using Core::Types::SubID;
    using Judge::LangID;
    using Core::Http::InternalServerError;
    using Core::Http::BadRequest;
    using Core::Http::OK;
    using Exceptions::ParameterException::ExceptionType::MISSING_PARAM;
    using Exceptions::ParameterException::ExceptionType::VALUE_ERROR;

    void submit(const httplib::Request &req, httplib::Response &res)
    {
        auto j = njson::parse(req.body);
        try {
            if (!j.contains("user_name") ||
                !j.contains("problem_title") || 
                !j.contains("source_code") || 
                !j.contains("language_id")) {
                    throw Exceptions::ParameterException(MISSING_PARAM, "submissions", "缺少提交信息");
            }

            auto& cfg{ App.getConfigurator() };
            JudgeDB::UserInquirer ui{
                cfg.get<std::string>("judgedb", "host", "127.0.0.1"),
                cfg.get<std::string>("judgedb", "username", "root"),
                cfg.get<std::string>("judgedb", "password"),
                cfg.get<std::string>("judgedb", "database", "judgedb")
            };
            if (!ui.isExist(j.at("user_name").get<std::string>())) {
                throw Exceptions::ParameterException(VALUE_ERROR, "user_name", "用户不存在");
            }

            JudgeDB::ProblemInquirer pi{
                cfg.get<std::string>("judgedb", "host", "127.0.0.1"),
                cfg.get<std::string>("judgedb", "username", "root"),
                cfg.get<std::string>("judgedb", "password"),
                cfg.get<std::string>("judgedb", "database", "judgedb")
            };
            if (!pi.isExist(j.at("problem_title").get<std::string>())) {
                throw Exceptions::ParameterException(VALUE_ERROR, "problem_title", "题目不存在");
            }

            Submission sub{
                .user_name = j.at("user_name").get<std::string>(),
                .problem_title = j.at("problem_title").get<std::string>(),
                .uploade_code_file_path = j.at("source_code").get<std::string>(),
                .submission_id =  boost::uuids::random_generator()(),
                .language_id = static_cast<LangID>(j.at("language_id").get<int>()),
            };
            std::string sub_id{ boost::uuids::to_string(sub.submission_id) };
            if (j.contains("compile_options")) {
                for (const auto &opt : j.at("compile_options")) {
                    sub.compile_options.push_back(opt.get<std::string>());
                }
            }

            if (!App.submit(std::move(sub))) {
                throw Exceptions::makeSystemException("judge submission error", __FILE__, __LINE__);
            }

            res.status = OK;
            auto response{ njson{{"status", "success"}, {"message", "提交成功"}, {"submission_id", sub_id}} };
            res.set_content(response.dump(), "application/json");
        } catch (const Exceptions::ParameterException& e) {
            res.status = BadRequest;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        } catch (const Exceptions::SystemException& e) {
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

