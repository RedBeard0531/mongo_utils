/**
*    Copyright (C) 2012 10gen Inc.
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
        uint64_t b = 0;
        for (unsigned i = 0; i < n; i++) {
            b += (cdc.readAndAdvance<LittleEndian<uint64_t>>() ^ i);
        }
        uint64_t c = 0;
        for (unsigned i = n * 2 * 8; i < len; i++) {  // 0-7 bytes left
            c = (c << 8) | ((const signed char*)buf)[i];
        }
        words[0] = a ^ len;
        words[1] = b ^ c;
    }

    bool operator==(const Checksum& rhs) const {
        return words[0] == rhs.words[0] && words[1] == rhs.words[1];
    }
    bool operator!=(const Checksum& rhs) const {
        return words[0] != rhs.words[0] || words[1] != rhs.words[1];
    }
};
}
