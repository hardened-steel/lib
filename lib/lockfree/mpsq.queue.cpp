#include <lib/lockfree/mpsq.queue.hpp>


namespace lib::lockfree {
    void MPSCQueue::Enqueue(Element* item) noexcept
    {
        item->next = nullptr;
        auto* prev = tail.exchange(item);
        std::atomic_ref(prev->next).store(item);
    }

    MPSCQueue::Element* MPSCQueue::Dequeue() noexcept
    {
        do {
            auto* head = this->head.load();
            auto* next = std::atomic_ref(head->next).load();

            if (head == &stub) {
                if (next == nullptr) {
                    return nullptr;
                }
                this->head.store(next);
                head = next;
                next = std::atomic_ref(next->next).load();
            }

            if (next != nullptr) {
                this->head.store(next);
                return head;
            }

            auto* tail = this->tail.load();
            if (head == tail) {
                Enqueue(&stub);
            }
        } while (true);
    }
}
