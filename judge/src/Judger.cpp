#include "Judger.hpp"

namespace Judge
{
    Judger::Judger(std::string_view base_url, std::string_view auth_token, int max_retries, int timeout, double backoff_factor)
        : m_ImplPtr{std::make_unique<Impl>(base_url, auth_token, max_retries, timeout, backoff_factor)}
    {
        if (!m_ImplPtr->cli.is_valid())
        {
            LOG_ERROR("httplib initialization failed, please check the URL.");
            throw std::runtime_error{"httplib initialization failed"};
        }
    }

    std::string Judger::submit(const httplib::Params &payload, bool is_base64)
    {
        auto sender{[this, payload, is_base64]() -> httplib::Result
                    {
                        std::string path{this->m_ImplPtr->judge_url + "/submissions/?"};
                        path += is_base64 ? "base64_endcoded=true" : "base64_endcoded=false";
                        return this->m_ImplPtr->cli.Post(path, payload);
                    }};

        auto result{this->requestWithRetry(sender)};
        if (!result)
            throw std::runtime_error{httplib::to_string(result.error())};

        auto response{this->requestWithRetry(sender).value()};
        return this->handleResponse(response).at("token").get<std::string>();
    }

    njson Judger::getJudgeResult(std::string_view token)
    {
        auto fetcher{[token, this]() -> httplib::Result
                     {
                         std::string path = "/submissions/" + std::string(token) + "/fields=*";
                         return this->m_ImplPtr->cli.Get(path);
                     }};

        auto result{this->requestWithRetry(fetcher)};
        if (!result)
            throw std::runtime_error{httplib::to_string(result.error())};

        auto response{this->requestWithRetry(fetcher).value()};
        return this->handleResponse(response);
    }

    njson Judger::handleResponse(const httplib::Response &resp)
    {
        if (resp.status < 200 || resp.status >= 300)
        {
            std::string err = "HTTP error: " + std::to_string(resp.status) + " " + resp.reason;
            if (resp.status == 429)
                err = " Submit too fast! Please try later.(Rate limit exceeded)";
            throw std::runtime_error{err};
        }
        try
        {
            return njson::parse(resp.body);
        }
        catch (njson::parse_error &ex)
        {
            std::stringstream ss;
            ss << "Error when parsing the response: " << resp.body << " --- " << ex.what();
            throw std::runtime_error(ss.str());
        }
        catch (njson::type_error &ex)
        {
            std::stringstream ss;
            ss << "Error when extracting the submission ID: " << resp.body << " --- " << ex.what();
            throw std::runtime_error(ss.str());
        }
        catch (std::exception &ex)
        {
            throw ex;
        }

        return njson::parse(resp.body);
    }

    Judger::Impl::Impl(std::string_view p_url,
                       std::string_view p_auth_token,
                       int p_max_etries,
                       int p_timeout,
                       double p_backoff_factor)
        : judge_url{std::string(p_url.back() == '/' ? p_url.substr(0, p_url.size() - 1) : p_url)},
          auth_token{std::string(p_auth_token)},
          timeout{p_timeout},
          max_retries{p_max_etries},
          backoff_factor{p_backoff_factor},
          cli{judge_url}
    {
        cli.set_read_timeout(timeout);
        cli.set_connection_timeout(timeout);
        cli.set_default_headers({{"Content-Type", "application/json"}});
        if (!auth_token.empty())
            cli.set_default_headers({{"X-Auth-Token", auth_token}});
    }
}