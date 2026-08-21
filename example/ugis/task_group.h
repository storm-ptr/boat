// Andrew Naplavkov

#ifndef TASK_GROUP_H
#define TASK_GROUP_H

#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>
#include <concepts>
#include <functional>
#include <list>
#include <stop_token>

class task_group {
public:
    explicit task_group(int num_threads = QThread::idealThreadCount())
    {
        pool_.setMaxThreadCount(num_threads);
        pool_.setThreadPriority(QThread::LowPriority);
    }

    ~task_group()
    {
        source_.request_stop();
        for (auto& fut : futures_)
            fut.waitForFinished();
    }

    template <std::invocable<std::stop_token> F>
    QFuture<void> run(F&& fn)
    {
        std::erase_if(futures_, [](auto& fut) { return fut.isFinished(); });
        auto fut = QtConcurrent::run(
            &pool_, [tok = source_.get_token(), fn = std::forward<F>(fn)] {
                std::invoke(fn, tok);
            });
        futures_.push_back(fut);
        return fut;
    }

    bool busy()
    {
        std::erase_if(futures_, [](auto& fut) { return fut.isFinished(); });
        return !futures_.empty();
    }

    void request_stop()
    {
        source_.request_stop();
        source_ = std::stop_source{};
    }

private:
    QThreadPool pool_;
    std::stop_source source_;
    std::list<QFuture<void>> futures_;
};

#endif  // TASK_GROUP_H
