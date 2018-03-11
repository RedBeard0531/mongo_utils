// processinfo.cpp

/*    Copyright 2009 10gen Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kControl

#include "mongo/platform/basic.h"

#include "mongo/base/init.h"
#include "mongo/util/processinfo.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <iostream>

#include "mongo/util/log.h"

using namespace std;

namespace mongo {

class PidFileWiper {
public:
    ~PidFileWiper() {
        if (path.empty()) {
            return;
        }

        ofstream out(path.c_str(), ios_base::out);
        out.close();
    }

    bool write(const boost::filesystem::path& p) {
        path = p;
        ofstream out(path.c_str(), ios_base::out);
        out << ProcessId::getCurrent() << endl;
        if (!out.good()) {
            auto errAndStr = errnoAndDescription();
            if (errAndStr.first == 0) {
                log() << "ERROR: Cannot write pid file to " << path.string()
                      << ": Unable to determine OS error";
            } else {
                log() << "ERROR: Cannot write pid file to " << path.string() << ": "
                      << errAndStr.second;
            }
        } else {
            boost::system::error_code ec;
            boost::filesystem::permissions(
                path,
                boost::filesystem::owner_read | boost::filesystem::owner_write |
                    boost::filesystem::group_read | boost::filesystem::others_read,
                ec);
            if (ec) {
                log() << "Could not set permissions on pid file " << path.string() << ": "
                      << ec.message();
                return false;
            }
        }
        return out.good();
    }

private:
    boost::filesystem::path path;
} pidFileWiper;

bool writePidFile(const string& path) {
    return pidFileWiper.write(path);
}

ProcessInfo::SystemInfo* ProcessInfo::systemInfo = NULL;

void ProcessInfo::initializeSystemInfo() {
    if (systemInfo == NULL) {
        systemInfo = new SystemInfo();
    }
}

/**
 * We need this get the system page size for the secure allocator, which the enterprise modules need
 * for storage for command line parameters.
 */
MONGO_INITIALIZER_GENERAL(SystemInfo, MONGO_NO_PREREQUISITES, MONGO_NO_DEPENDENTS)
(InitializerContext* context) {
    ProcessInfo::initializeSystemInfo();
    return Status::OK();
}
}
