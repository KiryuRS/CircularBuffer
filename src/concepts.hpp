#pragma once

#include <concepts>

template <typename T>
concept ring_compatible = requires {
    requires std::default_initializable<T>;
    requires std::movable<T>;
    requires std::copyable<T>;
};
