#include "api/api.hpp"

namespace OJApp
{
    void handleHttpWrapper(const httplib::Request &req, httplib::Response &res, std::initializer_list<const char*> require_params, ServiceHandler f) 
    {
        try {
            LOG_INFO("receive request {}", req.body);
            njson request = njson::parse(req.body);
            Exceptions::checkJsonParamExist(request, require_params);
            f(res, std::move(request));
            LOG_INFO("Response status {}, body {}", res.status, res.body);
        } catch (const ParameterException& e) {
            LOG_ERROR("Catch ParameterException: {}", e.what());
            res.status = BadRequest;
            res.set_content(njson{{"message", e.what()}}.dump(), "application/json");
        } catch (const njson::exception& e) {
            LOG_ERROR("Catch ParameterException: {}", e.what());
            res.status = BadRequest;
            res.set_content(njson{{"message", e.what()}}.dump(), "application/json");
        } catch (const SystemException& e) {
            LOG_ERROR("Catch ParameterException: {}", e.what());
            res.status = e.errorCode();
            res.set_content(njson{{"message", e.what()}}.dump(), "application/json");
        } catch (const std::exception& e) {
            LOG_ERROR("Catch ParameterException: {}", e.what());
            res.status = InternalServerError;
            res.set_content(njson{{"message", e.what()}}.dump(), "application/json");
        }
    }
}