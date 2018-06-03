/**
 *    Copyright (C) 2016 MongoDB Inc.
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

#include "mongo/stdx/memory.h"
#include "mongo/transport/message_compressor_noop.h"
#include "mongo/transport/message_compressor_registry.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/assert_util.h"

/**
 * Asserts that a value is null
 *
 * TODO: Move this into the unittest's standard ASSERT_ macros.
 */
#define ASSERT_NULL(a) ASSERT_EQ(a, static_cast<decltype(a)>(nullptr))

namespace mongo {
namespace {
TEST(MessageCompressorRegistry, RegularTest) {
    MessageCompressorRegistry registry;
    auto compressor = stdx::make_unique<NoopMessageCompressor>();
    auto compressorPtr = compressor.get();

    std::vector<std::string> compressorList = {compressorPtr->getName()};
    auto compressorListCheck = compressorList;
    registry.setSupportedCompressors(std::move(compressorList));
    registry.registerImplementation(std::move(compressor));
    registry.finalizeSupportedCompressors().transitional_ignore();

    ASSERT_TRUE(compressorListCheck == registry.getCompressorNames());

    ASSERT_EQ(registry.getCompressor(compressorPtr->getName()), compressorPtr);
    ASSERT_EQ(registry.getCompressor(compressorPtr->getId()), compressorPtr);

    ASSERT_NULL(registry.getCompressor("fakecompressor"));
    ASSERT_NULL(registry.getCompressor(255));
}

TEST(MessageCompressorRegistry, NothingRegistered) {
    MessageCompressorRegistry registry;

    ASSERT_NULL(registry.getCompressor("noop"));
    ASSERT_NULL(registry.getCompressor(0));
}

TEST(MessageCompressorRegistry, SetSupported) {
    MessageCompressorRegistry registry;
    auto compressor = stdx::make_unique<NoopMessageCompressor>();
    auto compressorId = compressor->getId();
    auto compressorName = compressor->getName();

    std::vector<std::string> compressorList = {"foobar"};
    registry.setSupportedCompressors(std::move(compressorList));
    registry.registerImplementation(std::move(compressor));
    auto ret = registry.finalizeSupportedCompressors();
    ASSERT_NOT_OK(ret);

    ASSERT_NULL(registry.getCompressor(compressorId));
    ASSERT_NULL(registry.getCompressor(compressorName));
}
}  // namespace
}  // namespace mongo
