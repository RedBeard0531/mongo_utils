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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kNetwork

#include "mongo/platform/basic.h"

#include "mongo/base/data_range_cursor.h"
#include "mongo/base/init.h"
#include "mongo/stdx/memory.h"
#include "mongo/transport/message_compressor_registry.h"
#include "mongo/transport/message_compressor_snappy.h"

#include <snappy.h>

namespace mongo {

SnappyMessageCompressor::SnappyMessageCompressor()
    : MessageCompressorBase(MessageCompressor::kSnappy) {}

std::size_t SnappyMessageCompressor::getMaxCompressedSize(size_t inputSize) {
    return snappy::MaxCompressedLength(inputSize);
}

StatusWith<std::size_t> SnappyMessageCompressor::compressData(ConstDataRange input,
                                                              DataRange output) {
    size_t outLength = output.length();
    if (output.length() < getMaxCompressedSize(input.length())) {
        return {ErrorCodes::BadValue, "Output too small for max size of compressed input"};
    }
    snappy::RawCompress(input.data(), input.length(), const_cast<char*>(output.data()), &outLength);

    counterHitCompress(input.length(), outLength);
    return {outLength};
}

StatusWith<std::size_t> SnappyMessageCompressor::decompressData(ConstDataRange input,
                                                                DataRange output) {
    size_t expectedLength = 0;
    if (!snappy::GetUncompressedLength(input.data(), input.length(), &expectedLength) ||
        expectedLength != output.length()) {
        return {ErrorCodes::BadValue, "Compressed message was invalid or corrupted"};
    }

    if (!snappy::RawUncompress(input.data(), input.length(), const_cast<char*>(output.data()))) {
        return Status{ErrorCodes::BadValue, "Compressed message was invalid or corrupted"};
    }

    counterHitDecompress(input.length(), output.length());
    return output.length();
}


MONGO_INITIALIZER_GENERAL(SnappyMessageCompressorInit,
                          ("EndStartupOptionHandling"),
                          ("AllCompressorsRegistered"))
(InitializerContext* context) {
    auto& compressorRegistry = MessageCompressorRegistry::get();
    compressorRegistry.registerImplementation(stdx::make_unique<SnappyMessageCompressor>());
    return Status::OK();
}
}  // namespace mongo
