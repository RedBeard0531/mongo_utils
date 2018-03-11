/*    Copyright 2014 10gen Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#pragma once

#include <atomic>
#include <cstring>
#include <type_traits>

#include "mongo/base/static_assert.h"
#include "mongo/stdx/type_traits.h"

namespace mongo {

/**
 * Instantiations of AtomicWord must be Integral, or Trivally Copyable and less than 8 bytes.
 */
template <typename _WordType, typename = void>
class AtomicWord;

/**
 * Implementation of the AtomicWord interface in terms of the C++11 Atomics.
 */
template <typename _WordType>
class AtomicWord<_WordType, stdx::enable_if_t<std::is_integral<_WordType>::value>> {
public:
    /**
     * Underlying value type.
     */
    typedef _WordType WordType;

    /**
     * Construct a new word with the given initial value.
     */
    explicit constexpr AtomicWord(WordType value = WordType(0)) : _value(value) {}

    /**
     * Gets the current value of this AtomicWord.
     *
     * Has acquire and release semantics.
     */
    WordType load() const {
        return _value.load();
    }

    /**
     * Gets the current value of this AtomicWord.
     *
     * Has relaxed semantics.
     */
    WordType loadRelaxed() const {
        return _value.load(std::memory_order_relaxed);
    }

    /**
     * Sets the value of this AtomicWord to "newValue".
     *
     * Has acquire and release semantics.
     */
    void store(WordType newValue) {
        return _value.store(newValue);
    }

    /**
     * Atomically swaps the current value of this with "newValue".
     *
     * Returns the old value.
     *
     * Has acquire and release semantics.
     */
    WordType swap(WordType newValue) {
        return _value.exchange(newValue);
    }

    /**
     * Atomic compare and swap.
     *
     * If this value equals "expected", sets this to "newValue".
     * Always returns the original of this.
     *
     * Has acquire and release semantics.
     */
    WordType compareAndSwap(WordType expected, WordType newValue) {
        // NOTE: Subtle: compare_exchange mutates its first argument.
        _value.compare_exchange_strong(expected, newValue);
        return expected;
    }

    /**
     * Get the current value of this, add "increment" and store it, atomically.
     *
     * Returns the value of this before incrementing.
     *
     * Has acquire and release semantics.
     */
    WordType fetchAndAdd(WordType increment) {
        return _value.fetch_add(increment);
    }

    /**
     * Get the current value of this, subtract "decrement" and store it, atomically.
     *
     * Returns the value of this before decrementing.
     *
     * Has acquire and release semantics.
     */
    WordType fetchAndSubtract(WordType decrement) {
        return _value.fetch_sub(decrement);
    }

    /**
     * Get the current value of this, add "increment" and store it, atomically.
     *
     * Returns the value of this after incrementing.
     *
     * Has acquire and release semantics.
     */
    WordType addAndFetch(WordType increment) {
        return fetchAndAdd(increment) + increment;
    }

    /**
     * Get the current value of this, subtract "decrement" and store it, atomically.
     *
     * Returns the value of this after decrementing.
     *
     * Has acquire and release semantics.
     */
    WordType subtractAndFetch(WordType decrement) {
        return fetchAndSubtract(decrement) - decrement;
    }

private:
    std::atomic<WordType> _value;  // NOLINT
};

/**
 * Implementation of the AtomicWord interface for non-integral types that are trivially copyable and
 * fit in 8 bytes.  For that implementation we flow reads and writes through memcpy'ing bytes in and
 * out of a uint64_t, then relying on std::atomic<uint64_t>.
 */
template <typename _WordType>
class AtomicWord<_WordType,
                 stdx::enable_if_t<sizeof(_WordType) <= sizeof(uint64_t) &&
                                   !std::is_integral<_WordType>::value &&
                                   std::is_trivially_copyable<_WordType>::value>> {
    using StorageType = uint64_t;

public:
    /**
     * Underlying value type.
     */
    typedef _WordType WordType;

    /**
     * Construct a new word with the given initial value.
     */
    explicit AtomicWord(WordType value = WordType{}) {
        store(value);
    }

    // Used in invoking a zero'd out non-integral atomic word
    struct ZeroInitTag {};
    /**
     * Construct a new word with zero'd out bytes.  Useful if you need a constexpr AtomicWord of
     * non-integral type.
     */
    constexpr explicit AtomicWord(ZeroInitTag) : _storage(0) {}

    /**
     * Gets the current value of this AtomicWord.
     *
     * Has acquire and release semantics.
     */
    WordType load() const {
        return _fromStorage(_storage.load());
    }

    /**
     * Gets the current value of this AtomicWord.
     *
     * Has relaxed semantics.
     */
    WordType loadRelaxed() const {
        return _fromStorage(_storage.load(std::memory_order_relaxed));
    }

    /**
     * Sets the value of this AtomicWord to "newValue".
     *
     * Has acquire and release semantics.
     */
    void store(WordType newValue) {
        _storage.store(_toStorage(newValue));
    }

    /**
     * Atomically swaps the current value of this with "newValue".
     *
     * Returns the old value.
     *
     * Has acquire and release semantics.
     */
    WordType swap(WordType newValue) {
        return _fromStorage(_storage.exchange(_toStorage(newValue)));
    }

    /**
     * Atomic compare and swap.
     *
     * If this value equals "expected", sets this to "newValue".
     * Always returns the original of this.
     *
     * Has acquire and release semantics.
     */
    WordType compareAndSwap(WordType expected, WordType newValue) {
        // NOTE: Subtle: compare_exchange mutates its first argument.
        auto v = _toStorage(expected);
        _storage.compare_exchange_strong(v, _toStorage(newValue));
        return _fromStorage(v);
    }

private:
    static WordType _fromStorage(StorageType storage) noexcept {
        WordType v;
        std::memcpy(&v, &storage, sizeof(v));
        return v;
    }

    static StorageType _toStorage(WordType wordType) noexcept {
        StorageType v = 0;
        std::memcpy(&v, &wordType, sizeof(wordType));
        return v;
    }

    std::atomic<StorageType> _storage;  // NOLINT
};

#define _ATOMIC_WORD_DECLARE(NAME, WTYPE)                       \
    typedef class AtomicWord<WTYPE> NAME;                       \
    namespace {                                                 \
    MONGO_STATIC_ASSERT(sizeof(NAME) == sizeof(WTYPE));         \
    MONGO_STATIC_ASSERT(std::is_standard_layout<WTYPE>::value); \
    }  // namespace

_ATOMIC_WORD_DECLARE(AtomicUInt32, unsigned);
_ATOMIC_WORD_DECLARE(AtomicUInt64, unsigned long long);
_ATOMIC_WORD_DECLARE(AtomicInt32, int);
_ATOMIC_WORD_DECLARE(AtomicInt64, long long);
_ATOMIC_WORD_DECLARE(AtomicBool, bool);
#undef _ATOMIC_WORD_DECLARE

}  // namespace mongo
