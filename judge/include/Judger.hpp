#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "Logger.hpp"


namespace Judge
{
    using njson = nlohmann::json;

    class Judger
    {
    public:
        Judger(std::string_view base_url, 
            std::string_view auth_token = "", 
            int max_retries = 3, 
            int timeout = 300, 
            double backoff_factor = 1.0);


        ~Judger() = default;


        std::string submit(const httplib::Params& payload, bool is_base64 = false);

        njson getJudgeResult(std::string_view token);

        struct Impl
        {
            int timeout;
            int max_retries;
            double backoff_factor;
            std::string judge_url;
            std::string auth_token;
            httplib::Client cli;

            Impl(std::string_view p_url, 
                std::string_view p_auth_token, 
                int p_max_etries, 
                int p_timeout, 
                double p_backoff_factor);
            ~Impl() noexcept = default;
        };

    private:
        njson handleResponse(const httplib::Response &resp);

        template <typename Func>
        requires std::invocable<Func> &&
                 std::same_as<std::invoke_result_t<Func>, httplib::Result>
        auto requestWithRetry(Func&& send_request)
        {
            for (int retry_cnt{0}; retry_cnt <= m_ImplPtr->max_retries; ++retry_cnt) {
                try {
                    return send_request();
                }
                catch (const std::exception &ex) {
                    if (retry_cnt >= m_ImplPtr->max_retries) {
                        LOG_ERROR("Request to {} failed: {}", m_ImplPtr->judge_url, ex.what());
                        break;
                    }
                    LOG_WARN(ex.what());
                    auto wait_time = m_ImplPtr->backoff_factor * std::pow(2, retry_cnt);
                    LOG_INFO("Judger: Waiting for {} seconds before the next request", wait_time);
                    std::this_thread::sleep_for(std::chrono::duration<double>(wait_time));
                }
            }
            throw std::runtime_error{ "Request failed" };
        }
    
    private:
        std::unique_ptr<Impl> m_ImplPtr;
    };
}