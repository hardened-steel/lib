#pragma once
#include <lib/raw.array.hpp>
#include <lib/fp/maybe.hpp>

#include <vector>


namespace lib::fp {
    template <class T>
    struct ListItem
    {
        T head;
        Maybe<ListItem> tail;
    };

    template <class T>
    using List = Maybe<ListItem<T>>;

    template <class T>
    List<T> MakeList(std::shared_ptr<std::vector<T>> array_ref, std::size_t index) // NOLINT
    {
        auto& array = *array_ref;
        if (index == array.size()) {
            co_return std::nullopt;
        } else {
            co_return ListItem<T> {array[index], MakeList(array_ref, index + 1)};
        }
    }

    template <class T>
    List<T> MakeList(std::vector<T> array)
    {
        auto array_ref = std::make_shared<std::vector<T>>(std::move(array));
        return MakeList(array_ref, 0);
    }

    template <class T>
    Maybe<T> head(List<T> list)
    {
        if (auto item = co_await list) {
            co_return item->head;
        }
        co_return std::nullopt;
    }

    template <class T>
    List<T> tail(List<T> list)
    {
        if (auto item = co_await list) {
            co_return co_await item->tail;
        }
        co_return std::nullopt;
    }

    template <class T>
    Val<bool> operator == (List<T> lhs, List<T> rhs) // NOLINT
    {
        const auto& l_item = co_await lhs;
        const auto& r_item = co_await rhs;

        if (l_item && r_item) {
            if (l_item->head == r_item->head) {
                co_return co_await (l_item->tail == r_item->tail);
            } else {
                co_return false;
            }
        }
        co_return true;
    }

    unittest {
        SimpleWorker worker;

        const auto list = MakeList<int>({1, 2, 3, 4});
        check(head(list)(worker) == 1);
        check(head(tail(list))(worker) == 2);
        check(head(tail(tail(list)))(worker) == 3);
        check(head(tail(tail(tail(list))))(worker) == 4);
        check(tail(tail(tail(tail(list))))(worker) == std::nullopt);
        check(head(tail(tail(tail(tail(list)))))(worker) == std::nullopt);
    }

    template <class From, class To>
    List<To> map(List<From> list, Fn<To(From)> convertor) // NOLINT
    {
        if (auto item = co_await list) {
            co_return ListItem<To> {
                co_await convertor(item->head),
                map(item->tail, convertor)
            };
        }
        co_return std::nullopt;
    }

    template <class From, class To>
    auto operator | (List<From> list, Fn<To(From)> convertor)
    {
        return map(list, convertor);
    }

    unittest {
        SimpleWorker worker;

        const Fn<int(int)> x2 = [](int value) {
            return value * 2;
        };

        const auto list = MakeList<int>({1, 2, 3});
        check(head(map(list, x2))(worker) == 2);
        check(((list | x2) == MakeList<int>({2, 4, 6}))(worker));
    }
}
