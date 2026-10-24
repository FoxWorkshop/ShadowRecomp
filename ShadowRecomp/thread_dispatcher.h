
// This class is used to wrap config option callback that require SDL3 update
// This is needed because those callback originally don't run in the main thread of our program

class MainThreadDispatcher
{
public:
    using Task = std::function<void()>;

    static MainThreadDispatcher& Get()
    {
        static MainThreadDispatcher instance;
        return instance;
    }

    // Called from any thread — queues work to run on the main thread
    void Post(Task task)
    {
        std::lock_guard lock(m_mutex);
        m_queue.push_back(std::move(task));
    }


    void Flush()
    {
        std::vector<Task> tasks;
        {
            std::lock_guard lock(m_mutex);
            std::swap(tasks, m_queue);
        }
        for (auto& task : tasks)
            task();
    }

private:
    std::mutex m_mutex;
    std::vector<Task> m_queue;
};