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

#include "mongo/transport/message_compressor_registry.h"

#include "mongo/base/init.h"
#include "mongo/stdx/memory.h"
#include "mongo/transport/message_compressor_noop.h"
#include "mongo/transport/message_compressor_snappy.h"
#include "mongo/transport/message_compressor_zlib.h"
#include "mongo/util/options_parser/option_section.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace mongo {
namespace {
const auto kDisabledConfigValue = "disabled"_sd;
const auto kDefaultConfigValue = "snappy"_sd;
}  // namespace

StringData getMessageCompressorName(MessageCompressor id) {
    switch (id) {
        case MessageCompressor::kNoop:
            return "noop"_sd;
        case MessageCompressor::kSnappy:
            return "snappy"_sd;
        case MessageCompressor::kZlib:
            return "zlib"_sd;
        default:
            fassert(40269, "Invalid message compressor ID");
    }
    MONGO_UNREACHABLE;
}

MessageCompressorRegistry& MessageCompressorRegistry::get() {
    static MessageCompressorRegistry globalRegistry;
    return globalRegistry;
}

void MessageCompressorRegistry::registerImplementation(
    std::unique_ptr<MessageCompressorBase> impl) {
    // It's an error to register a compressor that's already been registered
    fassert(40270,
            _compressorsByName.find(impl->getName()) == _compressorsByName.end() &&
                _compressorsByIds[impl->getId()] == nullptr);

    // Check to see if this compressor is allowed by configuration
    auto it = std::find(_compressorNames.begin(), _compressorNames.end(), impl->getName());
    if (it == _compressorNames.end())
        return;

    _compressorsByName[impl->getName()] = impl.get();
    _compressorsByIds[impl->getId()] = std::move(impl);
}

Status MessageCompressorRegistry::finalizeSupportedCompressors() {
    for (auto it = _compressorNames.begin(); it != _compressorNames.end(); ++it) {
        if (_compressorsByName.find(*it) == _compressorsByName.end()) {
            std::stringstream ss;
            ss << "Invalid network message compressor specified in configuration: " << *it;
            return {ErrorCodes::BadValue, ss.str()};
        }
    }
    return Status::OK();
}

const std::vector<std::string>& MessageCompressorRegistry::getCompressorNames() const {
    return _compressorNames;
}

MessageCompressorBase* MessageCompressorRegistry::getCompressor(MessageCompressorId id) const {
    return _compressorsByIds.at(id).get();
}

MessageCompressorBase* MessageCompressorRegistry::getCompressor(StringData name) const {
    auto it = _compressorsByName.find(name.toString());
    if (it == _compressorsByName.end())
        return nullptr;
    return it->second;
}

void MessageCompressorRegistry::setSupportedCompressors(std::vector<std::string>&& names) {
    _compressorNames = std::move(names);
}

Status addMessageCompressionOptions(moe::OptionSection* options, bool forShell) {
    auto& ret =
        options
            ->addOptionChaining("net.compression.compressors",
                                "networkMessageCompressors",
                                moe::String,
                                "Comma-separated list of compressors to use for network messages")
            .setImplicit(moe::Value(kDisabledConfigValue.toString()));
    if (forShell) {
        ret.setDefault(moe::Value(kDisabledConfigValue.toString())).hidden();
    } else {
        ret.setDefault(moe::Value(kDefaultConfigValue.toString()));
    }
    return Status::OK();
}

Status storeMessageCompressionOptions(const moe::Environment& params) {
    std::vector<std::string> restrict;
    if (params.count("net.compression.compressors")) {
        auto compressorListStr = params["net.compression.compressors"].as<std::string>();
        if (compressorListStr != kDisabledConfigValue) {
            boost::algorithm::split(restrict, compressorListStr, boost::is_any_of(", "));
        }
    }

    auto& compressorFactory = MessageCompressorRegistry::get();
    compressorFactory.setSupportedCompressors(std::move(restrict));

    return Status::OK();
}

// This instantiates and registers the "noop" compressor. It must happen after option storage
// because that's when the configuration of the compressors gets set.
MONGO_INITIALIZER_GENERAL(NoopMessageCompressorInit,
                          ("EndStartupOptionStorage"),
                          ("AllCompressorsRegistered"))
(InitializerContext* context) {
    auto& compressorRegistry = MessageCompressorRegistry::get();
    compressorRegistry.registerImplementation(stdx::make_unique<NoopMessageCompressor>());
    return Status::OK();
}

// This cleans up any compressors that were requested by the user, but weren't registered by
// any compressor. It must be run after all the compressors have registered themselves with
// the global registry.
MONGO_INITIALIZER(AllCompressorsRegistered)(InitializerContext* context) {
    return MessageCompressorRegistry::get().finalizeSupportedCompressors();
}
}  // namespace mongo
