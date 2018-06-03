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

#include "mongo/base/init.h"
#include "mongo/stdx/memory.h"
#include "mongo/transport/message_compressor_registry.h"
#include "mongo/transport/message_compressor_zlib.h"

#include <zlib.h>

namespace mongo {

ZlibMessageCompressor::ZlibMessageCompressor() : MessageCompressorBase(MessageCompressor::kZlib) {}

std::size_t ZlibMessageCompressor::getMaxCompressedSize(size_t inputSize) {
    return ::compressBound(inputSize);
}

StatusWith<std::size_t> ZlibMessageCompressor::compressData(ConstDataRange input,
                                                            DataRange output) {
    size_t outLength = output.length();
    int ret = ::compress2(const_cast<Bytef*>(reinterpret_cast<const Bytef*>(output.data())),
                          reinterpret_cast<uLongf*>(&outLength),
                          reinterpret_cast<const Bytef*>(input.data()),
                          input.length(),
                          Z_DEFAULT_COMPRESSION);

    if (ret != Z_OK) {
        return Status{ErrorCodes::BadValue, "Could not compress input"};
    }
    counterHitCompress(input.length(), outLength);
    return {outLength};
}

StatusWith<std::size_t> ZlibMessageCompressor::decompressData(ConstDataRange input,
                                                              DataRange output) {
    uLongf length = output.length();
    int ret = ::uncompress(const_cast<Bytef*>(reinterpret_cast<const Bytef*>(output.data())),
                           &length,
                           reinterpret_cast<const Bytef*>(input.data()),
                           input.length());

    if (ret != Z_OK) {
        return Status{ErrorCodes::BadValue, "Compressed message was invalid or corrupted"};
    }

    counterHitDecompress(input.length(), output.length());
    return {output.length()};
}


MONGO_INITIALIZER_GENERAL(ZlibMessageCompressorInit,
                          ("EndStartupOptionHandling"),
                          ("AllCompressorsRegistered"))
(InitializerContext* context) {
    auto& compressorRegistry = MessageCompressorRegistry::get();
    compressorRegistry.registerImplementation(stdx::make_unique<ZlibMessageCompressor>());
    return Status::OK();
}
}  // namespace mongo
