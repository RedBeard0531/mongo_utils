/**
 * Copyright (C) 2016 MongoDB Inc.
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
#include <string>
#include <vector>

#include "mongo/base/status.h"
#include "mongo/base/string_data.h"

namespace mongo {

class BSONObjBuilder;

namespace procparser {

/**
 * Reads a string matching /proc/stat format, and writes the values of the specified keys into
 * builder. If fields are not in the "data" parameter, they are omitted. Converts fields
 * from USER_HZ to milliseconds, and names fields with a "_ms" suffix. If the string is empty,
 * corrupt, or missing fields, the builder will simply be missing fields.
 *
 * keys - sorted vector of field names to include in the output, "cpu" will include the 11 fields
 *        that make up cpu. If keys is empty, all keys are outputed.
 * data - string to parsee
 * ticksPerSecond - USER_HZ value
 * builder - BSON output
 */
Status parseProcStat(const std::vector<StringData>& keys,
                     StringData data,
                     int64_t ticksPerSecond,
                     BSONObjBuilder* builder);

/**
* Read from file, and write the specified list of keys into builder.
*
* See parseProcStat.
*
* Returns Status errors on file reading issues.
*/
Status parseProcStatFile(StringData filename,
                         const std::vector<StringData>& keys,
                         BSONObjBuilder* builder);

/**
 * Read a string matching /proc/meminfo format, and write the specified list of keys in builder.
 *
 * keys - list of keys to output in BSON. If keys is empty, all keys are outputed.
 * data - string to parsee
 * builder - BSON output
 */
Status parseProcMemInfo(const std::vector<StringData>& keys,
                        StringData data,
                        BSONObjBuilder* builder);

/**
 * Read from file, and write the specified list of keys in builder.
 */
Status parseProcMemInfoFile(StringData filename,
                            const std::vector<StringData>& keys,
                            BSONObjBuilder* builder);

/**
 * Read a string matching /proc/net/netstat format, and write the keys
 * found in that string into builder.
 *
 * data - string to parse
 * builder - BSON output
 */
Status parseProcNetstat(const std::vector<StringData>& keys,
                        StringData data,
                        BSONObjBuilder* builder);

/**
 * Read from file, and write the keys found in that file into builder.
 */
Status parseProcNetstatFile(const std::vector<StringData>& keys,
                            StringData filename,
                            BSONObjBuilder* builder);

/**
 * Read a string matching /proc/diskstats format, and write the specified list of disks in builder.
 *
 * disks - vector of block devices to include in output. For each disk selected, 11 fields are
 *         output in a nested document. There is no error if the disk is not found in the data. Also
 *         a disk is excluded if it has no activity since startup (i.e. an idle CD-ROM drive). If
 *         disks is empty, all non-zero block devices are outputed (this will include partitions,
 *         etc).
 * data - string to parsee
 * builder - BSON output
 */
Status parseProcDiskStats(const std::vector<StringData>& disks,
                          StringData data,
                          BSONObjBuilder* builder);

/**
 * Read from file, and write the specified list of disks in builder.
 */
Status parseProcDiskStatsFile(StringData filename,
                              const std::vector<StringData>& disks,
                              BSONObjBuilder* builder);

/**
 * Get a vector of disks to monitor by enumerating the specified directory.
 *
 * If the directory does not exist, or otherwise permission is denied, returns an empty vector.
 */
std::vector<std::string> findPhysicalDisks(StringData directory);

}  // namespace procparser
}  // namespace mongo
