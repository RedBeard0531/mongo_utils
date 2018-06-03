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
#include "mongo/base/status_with.h"
#include "mongo/transport/message_compressor_base.h"
#include "mongo/transport/session.h"

#include <vector>

namespace mongo {

class BSONObj;
class BSONObjBuilder;
class Message;
class MessageCompressorRegistry;

class MessageCompressorManager {
    MONGO_DISALLOW_COPYING(MessageCompressorManager);

public:
    /*
     * Default constructor. Uses the global MessageCompressorRegistry.
     */
    MessageCompressorManager();

    /*
     * Constructs a manager from a specific MessageCompressorRegistry - used by the unit tests
     * to test various registry configurations.
     */
    explicit MessageCompressorManager(MessageCompressorRegistry* factory);

    MessageCompressorManager(MessageCompressorManager&&) = default;
    MessageCompressorManager& operator=(MessageCompressorManager&&) = default;

    /*
     * Called by a client constructing an isMaster request. This function will append the result
     * of _registry->getCompressorNames() to the BSONObjBuilder as a BSON array. If no compressors
     * are configured, it won't append anything.
     */
    void clientBegin(BSONObjBuilder* output);

    /*
     * Called by a client that has received an isMaster response (received after calling
     * clientBegin) and wants to finish negotiating compression.
     *
     * This looks for a BSON array called "compression" with the server's list of
     * requested algorithms. The first algorithm in that array will be used in subsequent calls
     * to compressMessage.
     */
    void clientFinish(const BSONObj& input);

    /*
     * Called by a server that has received an isMaster request.
     *
     * This looks for a BSON array called "compression" in input and appends the union of that
     * array and the result of _registry->getCompressorNames(). The first name in the compression
     * array in input will be used in subsequent calls to compressMessage
     *
     * If no compressors are configured that match those requested by the client, then it will
     * not append anything to the BSONObjBuilder output.
     */
    void serverNegotiate(const BSONObj& input, BSONObjBuilder* output);

    /*
     * Returns a new Message containing the compressed contentx of 'msg'. If compressorId is null,
     * then it selects the first negotiated compressor. Otherwise, it uses the compressor with the
     * given identifier. It is intended that this value echo back a value returned as the out
     * parameter value for compressorId from a call to decompressMessage.
     *
     * If _negotiated is empty (meaning compression was not negotiated or is not supported), then
     * it will return a ref-count bumped copy of the input message.
     *
     * If an error occurs in the compressor, it will return a Status error.
     */
    StatusWith<Message> compressMessage(const Message& msg,
                                        const MessageCompressorId* compressorId = nullptr);

    /*
     * Returns a new Message containing the decompressed copy of the input message.
     *
     * If the compressor specified in the input message is not supported, it will return a Status
     * error.
     *
     * If an error occurs in the compressor, it will return a Status error.
     *
     * This can be called before Compression is negotiated with the client. This class has a
     * pointer to the global MessageCompressorRegistry and can lookup any message's compressor
     * by ID number through that registry. As long as the compressor has been enabled process-wide,
     * it can decompress any message without negotiation.
     *
     * If the 'compressorId' parameter is non-null, it will be populated with the compressor
     * used. If 'decompressMessage' returns succesfully, then that value can be fed back into
     * compressMessage, ensuring that the same compressor is used on both sides of a conversation.
     */
    StatusWith<Message> decompressMessage(const Message& msg,
                                          MessageCompressorId* compressorId = nullptr);

    static MessageCompressorManager& forSession(const transport::SessionHandle& session);

private:
    std::vector<MessageCompressorBase*> _negotiated;
    MessageCompressorRegistry* _registry;
};

}  // namespace mongo
