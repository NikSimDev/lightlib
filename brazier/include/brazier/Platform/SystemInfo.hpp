#pragma once

namespace brazier::platform {

    int get_fd_limit() noexcept;

    int get_system_memory_mb() noexcept;

    int get_worker_count() noexcept;

    int get_thread_count() noexcept;

}