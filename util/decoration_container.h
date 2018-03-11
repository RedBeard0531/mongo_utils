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

#include <cstdint>
#include <memory>

namespace mongo {

template <typename DecoratedType>
class DecorationRegistry;

template <typename DecoratedType>
class Decorable;

/**
 * An container for decorations.
 */
template <typename DecoratedType>
class DecorationContainer {
    DecorationContainer(const DecorationContainer&) = delete;
    DecorationContainer& operator=(const DecorationContainer&) = delete;

public:
    /**
     * Opaque descriptor of a decoration.  It is an identifier to a field on the
     * DecorationContainer that is private to those modules that have access to the descriptor.
     */
    class DecorationDescriptor {
    public:
        DecorationDescriptor() = default;

    private:
        friend DecorationContainer;
        friend DecorationRegistry<DecoratedType>;
        friend Decorable<DecoratedType>;

        explicit DecorationDescriptor(size_t index) : _index(index) {}

        size_t _index;
    };

    /**
     * Opaque description of a decoration of specified type T.  It is an identifier to a field
     * on the DecorationContainer that is private to those modules that have access to the
     * descriptor.
     */
    template <typename T>
    class DecorationDescriptorWithType {
    public:
        DecorationDescriptorWithType() = default;

    private:
        friend DecorationContainer;
        friend DecorationRegistry<DecoratedType>;
        friend Decorable<DecoratedType>;

        explicit DecorationDescriptorWithType(DecorationDescriptor raw) : _raw(std::move(raw)) {}

        DecorationDescriptor _raw;
    };

    /**
     * Constructs a decorable built based on the given "registry."
     *
     * The registry must stay in scope for the lifetime of the DecorationContainer, and must not
     * have any declareDecoration() calls made on it while a DecorationContainer dependent on it
     * is in scope.
     */
    explicit DecorationContainer(Decorable<DecoratedType>* const decorated,
                                 const DecorationRegistry<DecoratedType>* const registry)
        : _registry(registry),
          _decorationData(new unsigned char[registry->getDecorationBufferSizeBytes()]) {
        // Because the decorations live in the externally allocated storage buffer at
        // `_decorationData`, there needs to be a way to get back from a known location within this
        // buffer to the type which owns those decorations.  We place a pointer to ourselves, a
        // "back link" in the front of this storage buffer, as this is the easiest "well known
        // location" to compute.
        Decorable<DecoratedType>** const backLink =
            reinterpret_cast<Decorable<DecoratedType>**>(_decorationData.get());
        *backLink = decorated;
        _registry->construct(this);
    }

    ~DecorationContainer() {
        _registry->destroy(this);
    }

    /**
     * Gets the decorated value for the given descriptor.
     *
     * The descriptor must be one returned from this DecorationContainer's associated _registry.
     */
    void* getDecoration(DecorationDescriptor descriptor) {
        return _decorationData.get() + descriptor._index;
    }

    /**
     * Same as the non-const form above, but returns a const result.
     */
    const void* getDecoration(DecorationDescriptor descriptor) const {
        return _decorationData.get() + descriptor._index;
    }

    /**
     * Gets the decorated value or the given typed descriptor.
     */
    template <typename T>
    T& getDecoration(DecorationDescriptorWithType<T> descriptor) {
        return *static_cast<T*>(getDecoration(descriptor._raw));
    }

    /**
     * Same as the non-const form above, but returns a const result.
     */
    template <typename T>
    const T& getDecoration(DecorationDescriptorWithType<T> descriptor) const {
        return *static_cast<const T*>(getDecoration(descriptor._raw));
    }

private:
    const DecorationRegistry<DecoratedType>* const _registry;
    const std::unique_ptr<unsigned char[]> _decorationData;
};

}  // namespace mongo
