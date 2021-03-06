/*    Copyright 2015 MongoDB Inc.
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

#include "mongo/platform/basic.h"

#include "mongo/platform/decimal128.h"

#include "mongo/util/assert_util.h"

namespace mongo {

Decimal128::Decimal128(int32_t int32Value) {
    MONGO_UNREACHABLE;
}

Decimal128::Decimal128(int64_t int64Value) {
    MONGO_UNREACHABLE;
}

Decimal128::Decimal128(double doubleValue,
                       RoundingPrecision roundPrecision,
                       RoundingMode roundMode) {
    MONGO_UNREACHABLE;
}

Decimal128::Decimal128(std::string stringValue, RoundingMode roundMode) {
    MONGO_UNREACHABLE;
}

Decimal128::Value Decimal128::getValue() const {
    MONGO_UNREACHABLE;
}

Decimal128 Decimal128::toAbs() const {
    MONGO_UNREACHABLE;
}

int32_t Decimal128::toInt(RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

int32_t Decimal128::toInt(uint32_t* signalingFlags, RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

int64_t Decimal128::toLong(RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

int64_t Decimal128::toLong(uint32_t* signalingFlags, RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

int32_t Decimal128::toIntExact(RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

int32_t Decimal128::toIntExact(uint32_t* signalingFlags, RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

int64_t Decimal128::toLongExact(RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

int64_t Decimal128::toLongExact(uint32_t* signalingFlags, RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

double Decimal128::toDouble(RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

double Decimal128::toDouble(uint32_t* signalingFlags, RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

std::string Decimal128::toString() const {
    MONGO_UNREACHABLE;
}

bool Decimal128::isZero() const {
    MONGO_UNREACHABLE;
}

bool Decimal128::isNaN() const {
    MONGO_UNREACHABLE;
}

bool Decimal128::isInfinite() const {
    MONGO_UNREACHABLE;
}

bool Decimal128::isNegative() const {
    MONGO_UNREACHABLE;
}

Decimal128 Decimal128::add(const Decimal128& other, RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

Decimal128 Decimal128::add(const Decimal128& other,
                           uint32_t* signalingFlags,
                           RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

Decimal128 Decimal128::subtract(const Decimal128& other, RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

Decimal128 Decimal128::subtract(const Decimal128& other,
                                uint32_t* signalingFlags,
                                RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

Decimal128 Decimal128::multiply(const Decimal128& other, RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

Decimal128 Decimal128::multiply(const Decimal128& other,
                                uint32_t* signalingFlags,
                                RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

Decimal128 Decimal128::divide(const Decimal128& other, RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

Decimal128 Decimal128::divide(const Decimal128& other,
                              uint32_t* signalingFlags,
                              RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

Decimal128 Decimal128::quantize(const Decimal128& other, RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

Decimal128 Decimal128::quantize(const Decimal128& reference,
                                uint32_t* signalingFlags,
                                RoundingMode roundMode) const {
    MONGO_UNREACHABLE;
}

Decimal128 Decimal128::normalize() const {
    MONGO_UNREACHABLE;
}

bool Decimal128::isEqual(const Decimal128& other) const {
    MONGO_UNREACHABLE;
}

bool Decimal128::isNotEqual(const Decimal128& other) const {
    MONGO_UNREACHABLE;
}

bool Decimal128::isGreater(const Decimal128& other) const {
    MONGO_UNREACHABLE;
}

bool Decimal128::isGreaterEqual(const Decimal128& other) const {
    MONGO_UNREACHABLE;
}

bool Decimal128::isLess(const Decimal128& other) const {
    MONGO_UNREACHABLE;
}

bool Decimal128::isLessEqual(const Decimal128& other) const {
    MONGO_UNREACHABLE;
}

const Decimal128 Decimal128::kLargestPositive = Decimal128();
const Decimal128 Decimal128::kSmallestPositive = Decimal128();
const Decimal128 Decimal128::kLargestNegative = Decimal128();
const Decimal128 Decimal128::kSmallestNegative = Decimal128();

const Decimal128 Decimal128::kLargestNegativeExponentZero = Decimal128();

const Decimal128 Decimal128::kPositiveInfinity = Decimal128();
const Decimal128 Decimal128::kNegativeInfinity = Decimal128();
const Decimal128 Decimal128::kPositiveNaN = Decimal128();
const Decimal128 Decimal128::kNegativeNaN = Decimal128();

const Decimal128 Decimal128::kNormalizedZero = {};

}  // namespace mongo
