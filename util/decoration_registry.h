/**
 *    Copyright (C) 2015 MongoDB Inc.
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

#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>
#include <vector>

#include "mongo/base/disallow_copying.h"
#include "mongo/base/static_assert.h"
#include "mongo/stdx/functional.h"
#include "mongo/util/decoration_container.h"
#include "mongo/util/scopeguard.h"

namespace mongo {

/**
 * Registry of decorations.
 *
 * A decoration registry corresponds to the "type" of a DecorationContainer.  For example, if
 * you have two registries, r1 and r2, a DecorationContainer constructed from r1 has instances
 * the decorations declared on r1, and a DecorationContainer constructed from r2 has instances
 * of the decorations declared on r2.
 */
template <typename DecoratedType>
class DecorationRegistry {
    MONGO_DISALLOW_COPYING(DecorationRegistry);

public:
    DecorationRegistry() = default;

    /**
     * Declares a decoration of type T, constructed with T's default constructor, and
     * returns a descriptor for accessing that decoration.
     *
     * NOTE: T's destructor must not throw exceptions.
     */
    template <typename T>
    auto declareDecoration() {
        MONGO_STATIC_ASSERT_MSG(std::is_nothrow_destructible<T>::value,
                                "Decorations must be nothrow destructible");
        return
            typename DecorationContainer<DecoratedType>::template DecorationDescriptorWithType<T>(
                std::move(declareDecoration(
                    sizeof(T), std::alignment_of<T>::value, &constructAt<T>, &destroyAt<T>)));
    }

    size_t getDecorationBufferSizeBytes() const {
        return _totalSizeBytes;
    }

    /**
     * Constructs the decorations declared in this registry on the given instance of
     * "decorable".
     *
     * Called by the DecorationContainer constructor. Do not call directly.
     */
    void construct(DecorationContainer<DecoratedType>* const container) const {
        using std::cbegin;

        auto iter = cbegin(_decorationInfo);

        auto cleanupFunction = [&iter, container, this ]() noexcept->void {
            using std::crend;
            std::for_each(std::make_reverse_iterator(iter),
                          crend(this->_decorationInfo),
                          [&](auto&& decoration) {
                              decoration.destructor(
                                  container->getDecoration(decoration.descriptor));
                          });
        };

        auto cleanup = MakeGuard(std::move(cleanupFunction));

        using std::cend;

        for (; iter != cend(_decorationInfo); ++iter) {
            iter->constructor(container->getDecoration(iter->descriptor));
        }

        cleanup.Dismiss();
    }

    /**
     * Destroys the decorations declared in this registry on the given instance of "decorable".
     *
     * Called by the DecorationContainer destructor.  Do not call directly.
     */
    void destroy(DecorationContainer<DecoratedType>* const container) const noexcept try {
        for (auto& decoration : _decorationInfo) {
            decoration.destructor(container->getDecoration(decoration.descriptor));
        }
    } catch (...) {
        std::terminate();
    }

private:
    /**
     * Function that constructs (initializes) a single instance of a decoration.
     */
    using DecorationConstructorFn = void (*)(void*);

    /**
     * Function that destroys (deinitializes) a single instance of a decoration.
     */
    using DecorationDestructorFn = void (*)(void*);

    struct DecorationInfo {
        DecorationInfo() {}
        DecorationInfo(
            typename DecorationContainer<DecoratedType>::DecorationDescriptor inDescriptor,
            DecorationConstructorFn inConstructor,
            DecorationDestructorFn inDestructor)
            : descriptor(std::move(inDescriptor)),
              constructor(std::move(inConstructor)),
              destructor(std::move(inDestructor)) {}

        typename DecorationContainer<DecoratedType>::DecorationDescriptor descriptor;
        DecorationConstructorFn constructor;
        DecorationDestructorFn destructor;
    };

    using DecorationInfoVector = std::vector<DecorationInfo>;

    template <typename T>
    static void constructAt(void* location) {
        new (location) T();
    }

    template <typename T>
    static void destroyAt(void* location) {
        static_cast<T*>(location)->~T();
    }

    /**
     * Declares a decoration with given "constructor" and "destructor" functions,
     * of "sizeBytes" bytes.
     *
     * NOTE: "destructor" must not throw exceptions.
     */
    typename DecorationContainer<DecoratedType>::DecorationDescriptor declareDecoration(
        const size_t sizeBytes,
        const size_t alignBytes,
        const DecorationConstructorFn constructor,
        const DecorationDestructorFn destructor) {
        const size_t misalignment = _totalSizeBytes % alignBytes;
        if (misalignment) {
            _totalSizeBytes += alignBytes - misalignment;
        }
        typename DecorationContainer<DecoratedType>::DecorationDescriptor result(_totalSizeBytes);
        _decorationInfo.push_back(DecorationInfo(result, constructor, destructor));
        _totalSizeBytes += sizeBytes;
        return result;
    }

    DecorationInfoVector _decorationInfo;
    size_t _totalSizeBytes{sizeof(void*)};
};

}  // namespace mongo
