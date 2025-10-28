#pragma once


namespace lib::data_structures {

    template <class Tag = void>
    class FLListElement;

    template <>
    struct FLListElement<void>
    {
        FLListElement* next = nullptr;

        ~FLListElement() noexcept;

        FLListElement() noexcept = default;
        FLListElement(FLListElement&& other) = delete;
    };
}
