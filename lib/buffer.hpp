#pragma once
#include <algorithm>
#include <memory>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <type_traits>
#include <string_view>

namespace lib::buffer {

    struct Last {};
    struct First {};

    constexpr static inline Last last {};
    constexpr static inline First first {};

    template<class T>
    class Fill;

    template<class T>
    class Read;

    template<class T>
    class View;
    
    template<class T>
    class ViewImpl
    {
    protected:
        T*          data_;
        std::size_t size_;
    public:
        using reference = T&;
        using const_reference = const T&;
        using iterator = T*;
        using const_iterator = const T*;
    public:
        constexpr ViewImpl(T* data = nullptr, std::size_t size = 0) noexcept
        : data_(data), size_(size)
        {}
        constexpr ViewImpl(const ViewImpl& other) noexcept
        : data_(other.data_), size_(other.size_)
        {}
        template<std::size_t Size>
        constexpr ViewImpl(T(&value)[Size]) noexcept
        : data_(value), size_(Size)
        {}
        ViewImpl& operator=(const ViewImpl& other) noexcept
        {
            if(this != &other) {
                data_ = other.data_;
                size_ = other.size_;
            }
            return *this;
        }
    public:
        constexpr auto begin() const noexcept
        {
            return data_;
        }
        constexpr auto end() const noexcept
        {
            return data_ + size_;
        }
        constexpr T& operator[](std::size_t index) const noexcept
        {
            return data_[index];
        }
    public:
        constexpr T* data() const noexcept
        {
            return data_;
        }
        constexpr std::size_t size() const noexcept
        {
            return size_;
        }
        constexpr bool empty() const noexcept
        {
            return !size_;
        }
    public:
        constexpr ViewImpl split(std::size_t first, std::size_t last) const noexcept
        {
            return ViewImpl(&data_[first], last - first);
        }
        constexpr ViewImpl split(First, std::size_t last) const noexcept
        {
            return split(0, last);
        }
        constexpr ViewImpl split(std::size_t first, Last) const noexcept
        {
            return split(first, size_ - 1u);
        }
        constexpr ViewImpl split(First, Last) const noexcept
        {
            return *this;
        }
    public:
        friend bool operator==(const ViewImpl& a, const ViewImpl& b) noexcept
        {
            return std::equal(a.begin(), a.end(), b.begin(), b.end());
        }
        friend bool operator!=(const ViewImpl& a, const ViewImpl& b) noexcept
        {
            return !std::equal(a.begin(), a.end(), b.begin(), b.end());
        }
    };

    template<class T>
    class Owner;

    template<class T>
    struct Resource: Resource<const T>
    {
        virtual T* data() noexcept = 0;
    };

    template<class T>
    struct Resource<const T>
    {
        virtual const T* data() const noexcept = 0;
        virtual std::size_t size() const noexcept = 0;
    };

    template<class T, class Container>
    struct ContainerResource: Resource<T>
    {
        Container container;
        T* data() noexcept override
        {
            return container.data();
        }
        const T* data() const noexcept override
        {
            return container.data();
        }
        std::size_t size() const noexcept override
        {
            return container.size();
        }
        constexpr ContainerResource(Container&& container) noexcept
        : container(std::move(container))
        {}
    };

    template<class T, class Container>
    struct ContainerResource<const T, Container>: Resource<const T>
    {
        Container container;
        const T* data() const noexcept override
        {
            return container.data();
        }
        std::size_t size() const noexcept override
        {
            return container.size();
        }
        constexpr ContainerResource(Container&& container) noexcept
        : container(std::move(container))
        {}
    };

    template<class T>
    class OwnerImpl: public ViewImpl<T>
    {
        template<class U>
        friend class Owner;
        using Base = ViewImpl<T>;
    protected:
        
        std::shared_ptr<Resource<T>> resource = nullptr;
    public:
        using Type = T;
    public:
        constexpr OwnerImpl(T* data, std::size_t size) noexcept
        : Base(data, size)
        {}
        template<class R>
        OwnerImpl(std::shared_ptr<R> resource) noexcept
        : Base(resource->data(), resource->size())
        , resource(std::move(resource))
        {}

        template<class R>
        OwnerImpl(std::shared_ptr<R> resource, ViewImpl<T> view) noexcept
        : Base(view)
        , resource(std::move(resource))
        {}

        template<class Container>
        explicit OwnerImpl(Container&& c)
        : OwnerImpl(std::make_shared<ContainerResource<T, std::decay_t<Container>>>(std::forward<Container>(c)))
        {}

        template<std::size_t Size>
        OwnerImpl(T(&value)[Size]) noexcept
        : Base(value, Size)
        {}

        OwnerImpl() = default;
        OwnerImpl(ViewImpl<T>) = delete;
        OwnerImpl(const OwnerImpl&) = delete;
        OwnerImpl(OwnerImpl&&) = default;
        OwnerImpl& operator=(const OwnerImpl&) = delete;
        OwnerImpl& operator=(OwnerImpl&&) = default;
    public:
        OwnerImpl share() const noexcept
        {
            return resource;
        }
        constexpr OwnerImpl split(std::size_t first, std::size_t last) const noexcept
        {
            return OwnerImpl(resource, ViewImpl<T>::split(first, last));
        }
        constexpr OwnerImpl split(First, std::size_t last) const noexcept
        {
            return split(0, last);
        }
        constexpr OwnerImpl split(std::size_t first, Last) const noexcept
        {
            return split(first, ViewImpl<T>::size() - 1u);
        }
        constexpr OwnerImpl split(First, Last) const noexcept
        {
            return share();
        }
    };

    template<class T>
    class View: public ViewImpl<T>
    {
    public:
        using ViewImpl<T>::ViewImpl;
        template<std::size_t N>
        View(std::array<T, N>& array) noexcept
        : ViewImpl<T>(array.data(), array.size())
        {}
        constexpr operator View<const T>() const noexcept
        {
            return View<const T>(ViewImpl<T>::data_, ViewImpl<T>::size_);
        }
    };

    template<>
    class View<const char>: public ViewImpl<const char>
    {
    public:
        using ViewImpl<const char>::ViewImpl;
        View(std::string_view view) noexcept
        : ViewImpl<const char>(view.data(), view.size())
        {}
    };

    template<class T>
    class View<const T>: public ViewImpl<const T>
    {
    public:
        using ViewImpl<const T>::ViewImpl;
        template<std::size_t N>
        View(const std::array<T, N>& array) noexcept
        : ViewImpl<const T>(array.data(), array.size())
        {}
    };

    template<class T, std::size_t N>
    View(T (&array)[N]) -> View<T>;

    template<class T, std::size_t N>
    View(const std::array<T, N>& array) -> View<const T>;

    template<class T, std::size_t N>
    View(std::array<T, N>& array) -> View<T>;

    template<std::size_t N>
    View(const char (&array)[N]) -> View<std::string_view>;

    template<class T>
    View(T*, std::size_t) -> View<T>;

    template<class T>
    class Owner: public OwnerImpl<T>
    {
    public:
        using OwnerImpl<T>::OwnerImpl;
        constexpr operator View<const T>() const noexcept
        {
            return View<const T>(OwnerImpl<T>::data(), OwnerImpl<T>::size());
        }
        Owner share() const noexcept
        {
            return OwnerImpl<T>::resource;
        }
    };

    template<>
    class Owner<const char>: public OwnerImpl<const char>
    {
    public:
        using OwnerImpl<const char>::OwnerImpl;
        constexpr Owner(std::string_view view) noexcept
        : OwnerImpl<const char>(view.data(), view.size())
        {}
        template<std::size_t Size>
        Owner(const char(&value)[Size]) noexcept
        : OwnerImpl<const char>(value, Size - 1)
        {}
        Owner(Owner<char>&& other) noexcept
        : OwnerImpl<const char>(std::move(other.resource), other)
        {}
        Owner share() const noexcept
        {
            return OwnerImpl<const char>::resource;
        }
    };

    template<class T>
    class Owner<const T>: public OwnerImpl<const T>
    {
    public:
        using OwnerImpl<const T>::OwnerImpl;
        Owner(Owner<T>&& other) noexcept
        : OwnerImpl<const T>(std::move(other.resource), other)
        {}
        Owner share() const noexcept
        {
            return OwnerImpl<const T>::resource;
        }
    };

    template<class T, std::size_t N>
    Owner(T (&array)[N]) -> Owner<T>;

    template<std::size_t N>
    Owner(const char (&array)[N]) -> Owner<std::string_view>;

    template<class T>
    Owner(T*, std::size_t) -> Owner<T>;

    template<class Container>
    Owner(Container&&) -> Owner<typename Container::value_type>;

    inline auto operator""_bytes(const char* str, std::size_t size) noexcept
    {
        return  ViewImpl<const std::byte>(reinterpret_cast<const std::byte*>(str), size);
    }
    inline auto operator""_view(const char* str, std::size_t size) noexcept
    {
        return  ViewImpl<const char>(str, size);
    }

    template<class T>
    class Fill
    {
        class Container: public Resource<T>
        {
            T* array;
            std::size_t count;
        public:
            Container(void* array) noexcept
            : array(static_cast<T*>(array)), count(0)
            {}
        public:
            T* data() noexcept override
            {
                return reinterpret_cast<T*>(array);
            }
            const T* data() const noexcept override
            {
                return reinterpret_cast<const T*>(array);
            }
            std::size_t size() const noexcept override
            {
                return count;
            }
            template<class ...TArgs>
            void put(TArgs&& ...args)
            {
                new(&array[count++]) T(std::forward<TArgs>(args)...);
            }
            ~Container() noexcept
            {
                for(std::size_t i = 0; i < count; ++i) {
                    std::destroy_at(&array[i]);
                }
            }
        };
        struct ContainerDeleter
        {
            void operator()(Container* container) const noexcept
            {
                if(container) {
                    std::destroy_at(container);
                    ::operator delete(container);
                }
            }
        };
        std::shared_ptr<Container> container;
        std::size_t capacity = 0;
    private:
        static std::unique_ptr<Container, ContainerDeleter> create(std::size_t size)
        {
            constexpr auto align = std::max(alignof(Container), alignof(T));
            const auto asize = sizeof(Container) + (align - alignof(Container)) + sizeof(T) * size;
            if(auto ptr = ::operator new(asize)) {
                new(ptr) Container(static_cast<std::byte*>(ptr) + sizeof(Container) + align - alignof(Container));
                return std::unique_ptr<Container, ContainerDeleter>(static_cast<Container*>(ptr), ContainerDeleter{});
            }
            throw std::bad_alloc();
        }
    public:
        Fill() = default;
        Fill(std::size_t size)
        : container(create(size))
        , capacity(size)
        {}
        Owner<T> reset(std::size_t size)
        {
            Owner<T> result(std::move(container));
            container = create(size);
            capacity = size;
            return result;
        }
        std::size_t size() const noexcept
        {
            return container ? container->size() : 0;
        }
        bool full() const noexcept
        {
            return size() == capacity;
        }
        bool empty() const noexcept
        {
            return size() == 0;
        }
        template<class ...TArgs>
        void put(TArgs&& ...args)
        {
            container->put(std::forward<TArgs>(args)...);
        }
    };

    template<class T>
    class Read
    {
        Owner<T> buffer;
        std::size_t      index;
    public:
        Read() noexcept
        : buffer(nullptr, 0)
        , index(0)
        {}
        Read(Owner<T> buffer) noexcept
        : buffer(std::move(buffer))
        , index(0)
        {}
        void reset(Owner<T> newdata)
        {
            buffer = std::move(newdata);
            index  = 0;
        }
        std::size_t size() const noexcept
        {
            return buffer.size() - index;
        }
        bool full() const noexcept
        {
            return index == 0;
        }
        bool empty() const noexcept
        {
            return size() == 0;
        }
        T& top() const noexcept
        {
            return buffer.data()[index];
        }
        T& get() noexcept
        {
            return buffer.data()[index++];
        }
    };
}
