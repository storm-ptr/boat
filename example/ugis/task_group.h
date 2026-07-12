// Andrew Naplavkov

#ifndef TASK_GROUP_H
#define TASK_GROUP_H

#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include <concepts>
#include <stop_token>
#include <utility>
#include <vector>

class task_group {
public:
    ~task_group()
    {
        source_.request_stop();
        for (auto& fut : futures_)
            fut.waitForFinished();
    }

    template <std::invocable<std::stop_token> F>
    void run(F&& fn)
    {
        std::erase_if(futures_, [](auto& fut) { return fut.isFinished(); });
        if (source_.stop_requested())
            source_ = std::stop_source{};
        auto tok = source_.get_token();
        futures_.push_back(
            QtConcurrent::run([tok, fn = std::forward<F>(fn)] { fn(tok); }));
    }

    bool busy()
    {
        std::erase_if(futures_, [](auto& fut) { return fut.isFinished(); });
        return !futures_.empty();
    }

    void request_stop() { source_.request_stop(); }

private:
    std::stop_source source_;
    std::vector<QFuture<void>> futures_;
};

#endif  // TASK_GROUP_H
