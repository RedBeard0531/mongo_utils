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

/**
 * This header describes a mechanism for making "decorable" types.
 *
 * A decorable type is one to which various subsystems may attach subsystem-private data, so long as
 * they declare what that data will be before any instances of the decorable type are created.
 *
 * For example, suppose you had a class Client, representing on a server a network connection to a
 * client process.  Suppose that your server has an authentication module, that attaches data to the
 * client about authentication.  If class Client looks something like this:
 *
 * class Client : public Decorable<Client>{
 * ...
 * };
 *
 * Then the authentication module, before the first client object is created, calls
 *
 *     const auto authDataDescriptor = Client::declareDecoration<AuthenticationPrivateData>();
 *
 * And stores authDataDescriptor in a module-global variable,
 *
 * And later, when it has a Client object, client, and wants to get at the per-client
 * AuthenticationPrivateData object, it calls
 *
 *    authDataDescriptor(client)
 *
 * to get a reference to the AuthenticationPrivateData for that client object.
 *
 * With this approach, individual subsystems get to privately augment the client object via
 * declarations local to the subsystem, rather than in the global client header.
 */

#pragma once

#include "mongo/util/decoration_container.h"
#include "mongo/util/decoration_registry.h"

namespace mongo {

template <typename D>
class Decorable {
    Decorable(const Decorable&) = delete;
    Decorable& operator=(const Decorable&) = delete;

public:
    template <typename T>
    class Decoration {
    public:
        Decoration() = delete;

        T& operator()(D& d) const {
            return static_cast<Decorable&>(d)._decorations.getDecoration(this->_raw);
        }

        T& operator()(D* const d) const {
            return (*this)(*d);
        }

        const T& operator()(const D& d) const {
            return static_cast<const Decorable&>(d)._decorations.getDecoration(this->_raw);
        }

        const T& operator()(const D* const d) const {
            return (*this)(*d);
        }

        const D* owner(const T* const t) const {
            return static_cast<const D*>(getOwnerImpl(t));
        }

        D* owner(T* const t) const {
            return static_cast<D*>(getOwnerImpl(t));
        }

        const D& owner(const T& t) const {
            return *owner(&t);
        }

        D& owner(T& t) const {
            return *owner(&t);
        }

    private:
        const Decorable* getOwnerImpl(const T* const t) const {
            return *reinterpret_cast<const Decorable* const*>(
                reinterpret_cast<const unsigned char* const>(t) - _raw._raw._index);
        }

        Decorable* getOwnerImpl(T* const t) const {
            return const_cast<Decorable*>(getOwnerImpl(const_cast<const T*>(t)));
        }

        friend class Decorable;

        explicit Decoration(
            typename DecorationContainer<D>::template DecorationDescriptorWithType<T> raw)
            : _raw(std::move(raw)) {}

        typename DecorationContainer<D>::template DecorationDescriptorWithType<T> _raw;
    };

    template <typename T>
    static Decoration<T> declareDecoration() {
        return Decoration<T>(getRegistry()->template declareDecoration<T>());
    }

protected:
    Decorable() : _decorations(this, getRegistry()) {}
    ~Decorable() = default;

private:
    static DecorationRegistry<D>* getRegistry() {
        static DecorationRegistry<D>* theRegistry = new DecorationRegistry<D>();
        return theRegistry;
    }

    DecorationContainer<D> _decorations;
};

}  // namespace mongo
