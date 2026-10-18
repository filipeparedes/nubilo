# concurrency

Tools for running work on several threads. For now it only holds the thread pool.

## Structure

| File | Role |
|---|---|
| `ThreadPool.h` / `ThreadPool.cpp` | Fixed set of worker threads that run submitted tasks from a shared queue. |
| `ThreadPoolError.h` | Error type returned by the pool (`ThreadPoolError{msg}`). |

## How it works

**Creating a pool.** `ThreadPool::create(n)` starts `n` worker threads. With 0 it uses the number of hardware threads. The constructor is private so that `create` can return a `unique_ptr` and report a failure (for example, if the operating system cannot start a thread) as an error instead of throwing.

**Submitting work.** `submit(task)` puts the task in a queue and returns a `std::future` with its result. An exception thrown by the task is stored in the future and rethrown by `get()`, so a failing task never stops a worker. Once the pool is stopping, `submit` returns an error.

**Workers.** Each worker sleeps on a condition variable until a task is queued. It then takes the task from the queue, releases the lock and runs the task, so several tasks run at the same time. The lock is only held while touching the queue.

**Shutdown.** The destructor marks the pool as stopping and wakes all workers. Each worker keeps taking tasks until the queue is empty and then exits, so tasks that were already queued still run. The destructor then waits for every thread to finish.

**In Nubilo.** `storeFile` (see the `blob` module) uses the pool to write the chunks of a file concurrently. `main` creates the pool at startup and passes it to the file routes.

## Notes

* **The pool is optional.** Callers that receive a `ThreadPool*` should also work with `nullptr`. If the pool cannot be created at startup, the server logs it and keeps running, storing chunks one after another.
* **Lifetime of the data.** The pool does not know what a task uses. If a task reads data owned by the caller, the caller must wait for the task's future before that data goes away.
* **Limits.** The number of threads is fixed when the pool is created. The queue has no size limit, and queued tasks cannot be cancelled or prioritised.