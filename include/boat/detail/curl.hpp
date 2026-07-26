// Andrew Naplavkov

#ifndef BOAT_CURL_HPP
#define BOAT_CURL_HPP

#include <curl/curl.h>
#include <boat/blob.hpp>
#include <boat/detail/config.hpp>
#include <map>

namespace boat {

class curl {
public:
    using value_type = std::pair<std::string, blob>;

    explicit curl(int connections)
    {
        static auto ec = curl_global_init(CURL_GLOBAL_ALL);
        check(ec);
        auto h = curl_multi_init();
        boat::check(h, "curl_multi_init");
        multi_.reset(h);
        check(
            curl_multi_setopt(h, CURLMOPT_MAX_TOTAL_CONNECTIONS, connections));
    }

    size_t size() const { return jobs_.size(); }

    void push(char const* url, char const* agent, int ssl)
    {
        auto h = curl_easy_init();
        boat::check(h, "curl_easy_init");
        auto easy = easy_ptr{h};
        auto val = std::make_unique<value_type>(url, blob{});
        check(curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1));
        check(curl_easy_setopt(h, CURLOPT_REFERER, url));
        check(curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, ssl));
        check(curl_easy_setopt(
            h, CURLOPT_TIMEOUT_MS, std::chrono::milliseconds{timeout}.count()));
        check(curl_easy_setopt(h, CURLOPT_URL, url));
        check(curl_easy_setopt(h, CURLOPT_USERAGENT, agent));
        check(curl_easy_setopt(h, CURLOPT_WRITEDATA, &val->second));
        check(curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, &callback));
        check(curl_multi_add_handle(multi_.get(), h));
        easy.get_deleter().multi = multi_.get();
        jobs_.insert({h, {std::move(easy), std::move(val)}});
    }

    value_type pop()
    {
        int count;
        do {
            check(curl_multi_perform(multi_.get(), &count));
        } while (count == int(jobs_.size()));
        CURLMsg* m;
        do {
            m = curl_multi_info_read(multi_.get(), &count);
            boat::check(m, "curl_multi_info_read");
        } while (m->msg != CURLMSG_DONE);
        check(m->data.result);
        auto it = jobs_.find(m->easy_handle);
        boat::check(it != jobs_.end(), "curl easy");
        return std::move(*jobs_.extract(it).mapped().second);
    }

private:
    struct del {
        CURLM* multi;

        void operator()(CURL* easy) const
        {
            if (multi && easy)
                curl_multi_remove_handle(multi, easy);
            curl_easy_cleanup(easy);
        }
    };

    using easy_ptr = std::unique_ptr<CURL, del>;

    unique_ptr<CURLM, curl_multi_cleanup> multi_;
    std::map<CURL*, std::pair<easy_ptr, std::unique_ptr<value_type>>> jobs_;

    static void check(CURLcode ec)
    {
        if (CURLE_OK != ec)
            throw std::runtime_error(curl_easy_strerror(ec));
    }

    static void check(CURLMcode ec)
    {
        if (CURLM_OK != ec)
            throw std::runtime_error(curl_multi_strerror(ec));
    }

    static size_t callback(void* ptr, size_t size, size_t nmemb, blob* buf)
    {
        buf->append_range(std::span{as_bytes(ptr), size * nmemb});
        return nmemb;
    }
};

}  // namespace boat

#endif  // BOAT_CURL_HPP
