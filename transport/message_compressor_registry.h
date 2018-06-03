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

#pragma once

#include "mongo/base/disallow_copying.h"
#include "mongo/base/status.h"
#include "mongo/transport/message_compressor_base.h"
#include "mongo/util/string_map.h"

#include <array>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mongo {

namespace optionenvironment {
class OptionSection;
class Environment;
}  // namespace option environment

namespace moe = mongo::optionenvironment;

/*
 * The MessageCompressorRegistry holds the global registrations of compressors for a process.
 */
class MessageCompressorRegistry {
    MONGO_DISALLOW_COPYING(MessageCompressorRegistry);

public:
    MessageCompressorRegistry() = default;

    MessageCompressorRegistry(MessageCompressorRegistry&&) = default;
    MessageCompressorRegistry& operator=(MessageCompressorRegistry&&) = default;

    /*
     * Returns the global MessageCompressorRegistry
     */
    static MessageCompressorRegistry& get();

    /*
     * Registers a new implementation of a MessageCompressor with the registry. This only gets
     * called during startup. It is an error to call this twice with compressors with the same name
     * or ID numbers.
     *
     * This method is not thread-safe and should only be called from a single-threaded context
     * (a MONGO_INITIALIZER).
     */
    void registerImplementation(std::unique_ptr<MessageCompressorBase> impl);

    /*
     * Returns the list of compressor names that have been registered and configured.
     *
     * Iterators and value in this vector may be invalidated by calls to:
     *   setSupportedCompressors
     *   finalizeSupportedCompressors
     */
    const std::vector<std::string>& getCompressorNames() const;

    /*
     * Returns a compressor given an ID number. If no compressor exists with the ID number, it
     * returns nullptr
     */
    MessageCompressorBase* getCompressor(MessageCompressorId id) const;

    /* Returns a compressor given a name. If no compressor with that name exists, it returns
     * nullptr
     */
    MessageCompressorBase* getCompressor(StringData name) const;

    /*
     * Sets the list of supported compressors for this registry. Should be called during
     * option parsing and before calling registerImplementation for any compressors.
     */
    void setSupportedCompressors(std::vector<std::string>&& compressorNames);

    /*
     * Finalizes the list of supported compressors for this registry. Should be called after all
     * calls to registerImplementation. It will remove any compressor names that aren't keys in
     * the _compressors map.
     */
    Status finalizeSupportedCompressors();

private:
    StringMap<MessageCompressorBase*> _compressorsByName;
    std::array<std::unique_ptr<MessageCompressorBase>,
               std::numeric_limits<MessageCompressorId>::max() + 1>
        _compressorsByIds;
    std::vector<std::string> _compressorNames;
};

Status addMessageCompressionOptions(moe::OptionSection* options, bool forShell);
Status storeMessageCompressionOptions(const moe::Environment& params);
void appendMessageCompressionStats(BSONObjBuilder* b);
}  // namespace mongo
