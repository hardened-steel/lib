#pragma once
#include <iterator>


namespace lib::data_structures {

    template <class Tag = void>
    struct DLListElement;

    template <>
    struct DLListElement<void>
    {
        DLListElement* next = nullptr;
        DLListElement* prev = nullptr;

        ~DLListElement() noexcept;

        DLListElement() noexcept = default;
        DLListElement(DLListElement&& other) = delete;
    };

    template <class Tag>
    struct DLListElement: DLListElement<void>
    {
    };

    inline void AddItemAfter(DLListElement<>& item, DLListElement<>* after) noexcept
    {
        item.prev = after;
        item.next = nullptr;
        if (after != nullptr) {
            item.next = after->next;
            after->next = &item;
        }
        if (item.next != nullptr) {
            item.next->prev = &item;
        }
    }

    inline void AddItemBefore(DLListElement<>& item, DLListElement<>* before) noexcept
    {
        item.next = before;
        item.prev = nullptr;
        if (before != nullptr) {
            item.prev = before->prev;
            before->prev = &item;
        }
        if (item.prev != nullptr) {
            item.prev->next = &item;
        }
    }

    inline void RemoveItem(DLListElement<>& item) noexcept
    {
        if (item.next != nullptr) {
            item.next->prev = item.prev;
        }
        if (item.prev != nullptr) {
            item.prev->next = item.next;
        }
        item.next = nullptr;
        item.prev = nullptr;
    }

    inline DLListElement<>::~DLListElement() noexcept
    {
        RemoveItem(*this);
    }

    class DLList
    {
        DLListElement<>* head = nullptr;
        DLListElement<>* tail = nullptr;

    public:
        void PushBack(DLListElement<>& item) noexcept
        {
            AddItemAfter(item, tail);
            tail = &item;
            if (head == nullptr) {
                head = &item;
            }
        }

        template <class Tag>
        void PushBack(DLListElement<Tag>& item) noexcept
        {
            PushBack(static_cast<DLListElement<>&>(item));
        }

        void PushFront(DLListElement<>& item) noexcept
        {
            AddItemBefore(item, head);
            head = &item;
            if (tail == nullptr) {
                tail = &item;
            }
        }

        template <class Tag>
        void PushFront(DLListElement<Tag>& item) noexcept
        {
            PushFront(static_cast<DLListElement<>&>(item));
        }

        template <class T>
        struct TRange
        {
            const DLList& list;

            static_assert(std::is_base_of_v<DLListElement<>, T>);

            class Iterator
            {
                DLListElement<>* item;

            public:
                using iterator_category = std::bidirectional_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = T;
                using pointer           = value_type*;
                using reference         = value_type&;

            public:
                explicit Iterator(DLListElement<>* item) noexcept
                : item(item)
                {}

                reference operator*() const noexcept
                {
                    return static_cast<T&>(*item);
                }

                pointer operator -> () const noexcept
                {
                    return static_cast<T*>(item);
                }

                Iterator& operator++()
                {
                    item = item->next;
                    return *this;
                }

                Iterator& operator--()
                {
                    item = item->prev;
                    return *this;
                }

                friend bool operator==(const Iterator& lhs, const Iterator& rhs) noexcept
                {
                    return lhs.item == rhs.item;
                }

                friend bool operator!=(const Iterator& lhs, const Iterator& rhs) noexcept
                {
                    return lhs.item != rhs.item;
                }
            };

            using iterator = Iterator;

            auto begin() noexcept
            {
                return iterator(list.head);
            }

            auto end() noexcept // NOLINT
            {
                return iterator(nullptr);
            }
        };

        template <class T>
        TRange<T> Range() noexcept
        {
            return {*this};
        }

        template <class T>
        TRange<const T> Range() const noexcept
        {
            return {*this};
        }
    };

}