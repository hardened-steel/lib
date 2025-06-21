#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

template <class Key, class Value>
class HashTable
{
    using Item = std::pair<const Key, Value>;
    std::vector<std::unique_ptr<Item>> m_items;
    std::size_t m_size = 0;
public:
    class Iterator
    {
        HashTable& table;
        std::size_t index;
    public:
        Iterator(HashTable& table, std::size_t index) noexcept
        : table(table), index(index)
        {}

        auto& operator*() const noexcept
        {
            return *(table.m_items[index]);
        }
        Iterator& operator++() noexcept
        {
            do {
                index += 1;
            } while((index < table.m_items.size()) && (!table.m_items[index]));
            return *this;
        }
        friend bool operator==(const Iterator& lhs, const Iterator& rhs) noexcept
        {
            return (&lhs.table == &rhs.table) && (lhs.index == rhs.index);
        }
        friend bool operator==(const Iterator& lhs, const Iterator& rhs) noexcept
        {
            return (&lhs.table != &rhs.table) || (lhs.index != rhs.index);
        }
    };
public:
    void insert(const Key& key, const Value& value)
    {
        if (m_size == m_items.size()) {
            rehash();
        }
        auto item = create_item(key, value);
        const std::hash<Key> hash_function;
        const auto hash = hash_function(item->first);
        while (m_items[hash % m_items.size()]) {
            hash += 1;
        }
        m_items[hash % m_items.size()] = std::move(item);
    }

    Iterator find(const Key& key) noexcept
    {
        const std::hash<Key> hash_function;

        const auto hash = hash_function(key);
        for (std::size_t i = 0; i < m_size; ++i) {
            const auto index = (hash + i) % m_items.size();
            auto& item = m_items[index];
            if (item) {
                if (item->first == key) {
                    return Iterator(*this, index);
                }
            } else {
                break;
            }
        }
        return end();
    }
    auto begin() noexcept
    {
        Iterator it(*this, 0);
        if (!m_items.empty()) {
            if (!m_items[0]) {
                ++it;
            }
        }
        return it;
    }
    auto end() noexcept
    {
        return Iterator(*this, m_items.size());
    }
private:
    auto create_item(const Key& key, const Value& value)
    {
        return std::make_unique<Item>(key, value);
    }
    void rehash()
    {
        const std::hash<Key> hash_function;

        std::vector<std::unique_ptr<Item>> items;
        items.resize((m_items.size() + 1) * 2);
        for (auto& item: m_items) {
            if (item) {
                const auto hash = hash_function(item->first);
                while (items[hash % m_items.size()]) {
                    hash += 1;
                }
                items[hash % items.size()] = std::move(item);
            }
        }
        m_items = std::move(items);
    }
};