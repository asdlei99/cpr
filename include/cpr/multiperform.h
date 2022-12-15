#ifndef CPR_MULTIPERFORM_H
#define CPR_MULTIPERFORM_H

#include "cpr/curlmultiholder.h"
#include "cpr/response.h"
#include "cpr/session.h"
#include <functional>
#include <memory>
#include <queue>
#include <stdexcept>
#include <vector>

namespace cpr {

class InterceptorMulti;

class MultiPerform {
  public:
    enum class HttpMethod {
        UNDEFINED = 0,
        GET_REQUEST,
        POST_REQUEST,
        PUT_REQUEST,
        DELETE_REQUEST,
        PATCH_REQUEST,
        HEAD_REQUEST,
        OPTIONS_REQUEST,
        DOWNLOAD_REQUEST,
    };

    MultiPerform();
    ~MultiPerform();

    std::vector<Response> Get();
    std::vector<Response> Delete();
    template <typename... DownloadArgTypes>
    std::vector<Response> Download(DownloadArgTypes... args);
    std::vector<Response> Put();
    std::vector<Response> Head();
    std::vector<Response> Options();
    std::vector<Response> Patch();
    std::vector<Response> Post();

    std::vector<Response> Perform();
    template <typename... DownloadArgTypes>
    std::vector<Response> PerformDownload(DownloadArgTypes... args);

    void AddSession(std::shared_ptr<Session>& session, HttpMethod method = HttpMethod::UNDEFINED);
    void RemoveSession(const std::shared_ptr<Session>& session);

    void AddInterceptor(const std::shared_ptr<InterceptorMulti>& pinterceptor);

  private:
    void SetHttpMethod(HttpMethod method);

    void PrepareSessions();
    template <typename CurrentDownloadArgType, typename... DownloadArgTypes>
    void PrepareDownloadSessions(size_t sessions_index, CurrentDownloadArgType current_arg, DownloadArgTypes... args);
    template <typename CurrentDownloadArgType>
    void PrepareDownloadSessions(size_t sessions_index, CurrentDownloadArgType current_arg);
    void PrepareDownloadSession(size_t sessions_index, std::ofstream& file);
    void PrepareDownloadSession(size_t sessions_index, const WriteCallback& write);

    void PrepareGet();
    void PrepareDelete();
    void PreparePut();
    void PreparePatch();
    void PrepareHead();
    void PrepareOptions();
    void PreparePost();
    template <typename... DownloadArgTypes>
    void PrepareDownload(DownloadArgTypes... args);

    std::vector<Response> intercept();
    std::vector<Response> proceed();
    std::vector<Response> MakeRequest();
    std::vector<Response> MakeDownloadRequest();

    void DoMultiPerform();
    std::vector<Response> ReadMultiInfo(std::function<Response(Session&, CURLcode)>&& complete_function);

    std::vector<std::pair<std::shared_ptr<Session>, HttpMethod>> sessions_;
    std::unique_ptr<CurlMultiHolder> multicurl_;
    bool is_download_multi_perform{false};

    // Interceptors should be able to call the private proceed() function
    friend InterceptorMulti;
    std::queue<std::shared_ptr<InterceptorMulti>> interceptors_;
};

template <typename CurrentDownloadArgType>
void MultiPerform::PrepareDownloadSessions(size_t sessions_index, CurrentDownloadArgType current_arg) {
    PrepareDownloadSession(sessions_index, current_arg);
}

template <typename CurrentDownloadArgType, typename... DownloadArgTypes>
void MultiPerform::PrepareDownloadSessions(size_t sessions_index, CurrentDownloadArgType current_arg, DownloadArgTypes... args) {
    PrepareDownloadSession(sessions_index, current_arg);
    PrepareDownloadSessions<DownloadArgTypes...>(sessions_index + 1, args...);
}


template <typename... DownloadArgTypes>
void MultiPerform::PrepareDownload(DownloadArgTypes... args) {
    SetHttpMethod(HttpMethod::DOWNLOAD_REQUEST);
    PrepareDownloadSessions<DownloadArgTypes...>(0, args...);
}

template <typename... DownloadArgTypes>
std::vector<Response> MultiPerform::Download(DownloadArgTypes... args) {
    if (sizeof...(args) != sessions_.size()) {
        throw std::invalid_argument("Number of download arguments has to match the number of sessions added to the multiperform!");
    }
    PrepareDownload(args...);
    return MakeDownloadRequest();
}

template <typename... DownloadArgTypes>
std::vector<Response> MultiPerform::PerformDownload(DownloadArgTypes... args) {
    if (sizeof...(args) != sessions_.size()) {
        throw std::invalid_argument("Number of download arguments has to match the number of sessions added to the multiperform!");
    }
    PrepareDownloadSessions<DownloadArgTypes...>(0, args...);
    return MakeDownloadRequest();
}

} // namespace cpr

#endif