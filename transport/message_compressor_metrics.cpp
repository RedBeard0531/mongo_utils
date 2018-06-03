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

#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/transport/message_compressor_registry.h"

namespace mongo {
namespace {
const auto kBytesIn = "bytesIn"_sd;
const auto kBytesOut = "bytesOut"_sd;
}  // namespace

void appendMessageCompressionStats(BSONObjBuilder* b) {
    auto& registry = MessageCompressorRegistry::get();
    const auto& names = registry.getCompressorNames();
    if (names.empty()) {
        return;
    }
    BSONObjBuilder compressionSection(b->subobjStart("compression"));

    for (auto&& name : names) {
        auto&& compressor = registry.getCompressor(name);
        BSONObjBuilder base(compressionSection.subobjStart(name));

        BSONObjBuilder compressorSection(base.subobjStart("compressor"));
        compressorSection << kBytesIn << compressor->getCompressorBytesIn() << kBytesOut
                          << compressor->getCompressorBytesOut();
        compressorSection.doneFast();

        BSONObjBuilder decompressorSection(base.subobjStart("decompressor"));
        decompressorSection << kBytesIn << compressor->getDecompressorBytesIn() << kBytesOut
                            << compressor->getDecompressorBytesOut();
        decompressorSection.doneFast();
        base.doneFast();
    }
    compressionSection.doneFast();
}

}  // namespace mongo
