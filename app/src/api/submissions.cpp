#include <nlohmann/json.hpp>
#include "Application.hpp"
#include "api/submissions.hpp"
#include "utilities/Submission.hpp"
#include "utilities/Language.hpp"
#include "Types.hpp"

namespace OJApp::Submissions
{
    using njson = nlohmann::json;
    using Judge::Submission;
    using Core::Types::SubID;
    using Judge::LangID;

    void submit(const httplib::Request &req, httplib::Response &res)
    {
        auto j = njson::parse(req.body);
        Submission sub{};
        try {
            sub.problem_title = j.at("problem_title").get<std::string>();
            sub.uploade_code_file_path = j.at("source_code").get<std::string>(); // 先写入源码 , 后被替换为文件路径
            for (const auto& opt : j.at("compile_options")) {
                sub.compile_options.push_back(opt.get<std::string>());
            }
            sub.language_id = static_cast<LangID>(j.at("language_id").get<int>());
            
            sub.submission_id =  boost::uuids::random_generator()();
            res.set_content(boost::uuids::to_string(sub.submission_id), "text/plain");
            if (!App.submit(std::move(sub))) {
                res.status = 500;
                res.set_content("Unexpected exception", "text/plain");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content("Missing submission fields", "text/plain");
            return;
        }
        res.status = 200;
    }
}
