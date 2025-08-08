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
        try {
            njson json = njson::parse(req.body);
            if (!json.contains("username") ||
                !json.contains("problem_title") || 
                !json.contains("source_code") || 
                !json.contains("language_id")) {
                    throw Exceptions::ParameterException(MISSING_PARAM, "submissions", "Missing parameters");
            }

            auto& cfg{ App.getConfigurator() };
            JudgeDB::UserInquirer ui{
                cfg.get<std::string>("judgedb", "HOST", "127.0.0.1"),
                cfg.get<uint16_t>("judgedb", "PORT", 3306),
                cfg.get<std::string>("judgedb", "USERNAME", "root"),
                cfg.get<std::string>("judgedb", "PASSWORD"),
                cfg.get<std::string>("judgedb", "DATABASE", "judgedb")
            };
            if (!ui.isExist(json.at("username").get<std::string>())) {
                throw Exceptions::ParameterException(VALUE_ERROR, "username", "User doesn't exist");
            }

            JudgeDB::ProblemInquirer pi{
                cfg.get<std::string>("judgedb", "HOST", "127.0.0.1"),
                cfg.get<uint16_t>("judgedb", "PORT", 3306),
                cfg.get<std::string>("judgedb", "USERNAME", "root"),
                cfg.get<std::string>("judgedb", "PASSWORD"),
                cfg.get<std::string>("judgedb", "DATABASE", "judgedb")
            };
            if (!pi.isExist(json.at("problem_title").get<std::string>())) {
                throw Exceptions::ParameterException(VALUE_ERROR, "problem_title", "Problem title doesn't exist");
            }

            Submission sub{
                .username = json.at("username").get<std::string>(),
                .problem_title = json.at("problem_title").get<std::string>(),
                .uploade_code_file_path = json.at("source_code").get<std::string>(),
                .submission_id =  boost::uuids::random_generator()(),
                .language_id = static_cast<LangID>(json.at("language_id").get<int>()),
            };
            std::string sub_id{ boost::uuids::to_string(sub.submission_id) };
            if (json.contains("compile_options")) {
                for (const auto &opt : json.at("compile_options")) {
                    sub.compile_options.push_back(opt.get<std::string>());
                }
            }

            if (!App.submit(std::move(sub))) {
                throw Exceptions::makeSystemException("judge submission error");
            }

            res.status = OK;
            auto response = njson{{"status", "success"}, {"message", "submission succeed"}, {"submission_id", sub_id}};
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