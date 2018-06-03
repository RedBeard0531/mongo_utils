// random.h

/*    Copyright 2012 10gen Inc.
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
#include <limits>
#include <memory>

namespace mongo {

/**
 * Uses http://en.wikipedia.org/wiki/Xorshift
 */
class PseudoRandom {
public:
    PseudoRandom(int32_t seed);

    PseudoRandom(uint32_t seed);

    PseudoRandom(int64_t seed);

    int32_t nextInt32();

    int64_t nextInt64();

    /**
     * Returns a random number in the range [0, 1).
     */
    double nextCanonicalDouble();

    /**
     * @return a number between 0 and max
     */
    int32_t nextInt32(int32_t max) {
        return static_cast<uint32_t>(nextInt32()) % static_cast<uint32_t>(max);
    }

    /**
     * @return a number between 0 and max
     */
    int64_t nextInt64(int64_t max) {
        return static_cast<uint64_t>(nextInt64()) % static_cast<uint64_t>(max);
    }

    /**
     * This returns an object that adapts PseudoRandom such that it
     * can be used as the third argument to std::shuffle. Note that
     * the lifetime of the returned object must be a subset of the
     * lifetime of the PseudoRandom object.
     */
    auto urbg() {

        class URBG {
        public:
            explicit URBG(PseudoRandom* impl) : _impl(impl) {}

            using result_type = uint64_t;

            static constexpr result_type min() {
                return std::numeric_limits<result_type>::min();
            }

            static constexpr result_type max() {
                return std::numeric_limits<result_type>::max();
            }

            result_type operator()() {
                return _impl->nextInt64();
            }

        private:
            PseudoRandom* _impl;
        };

        return URBG(this);
    }

private:
    uint32_t nextUInt32();

    uint32_t _x;
    uint32_t _y;
    uint32_t _z;
    uint32_t _w;
};

/**
 * More secure random numbers
 * Suitable for nonce/crypto
 * Slower than PseudoRandom, so only use when really need
 */
class SecureRandom {
public:
    virtual ~SecureRandom();

    virtual int64_t nextInt64() = 0;

    static std::unique_ptr<SecureRandom> create();
};
}  // namespace mongo
