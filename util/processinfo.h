// processinfo.h

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

#pragma once

#include <boost/optional.hpp>
#include <cstdint>
#include <string>

#include "mongo/db/jsobj.h"
#include "mongo/platform/process_id.h"
#include "mongo/stdx/mutex.h"
#include "mongo/util/concurrency/mutex.h"

namespace mongo {

class ProcessInfo {
public:
    ProcessInfo(ProcessId pid = ProcessId::getCurrent());
    ~ProcessInfo();

    /**
     * @return mbytes
     */
    int getVirtualMemorySize();

    /**
     * @return mbytes
     */
    int getResidentSize();

    /**
     * Get the type of os (e.g. Windows, Linux, Mac OS)
     */
    static const std::string& getOsType() {
        return sysInfo().osType;
    }

    /**
     * Get the os Name (e.g. Ubuntu, Gentoo, Windows Server 2008)
     */
    static const std::string& getOsName() {
        return sysInfo().osName;
    }

    /**
     * Get the os version (e.g. 10.04, 11.3.0, 6.1 (build 7600))
     */
    static const std::string& getOsVersion() {
        return sysInfo().osVersion;
    }

    /**
     * Get the cpu address size (e.g. 32, 36, 64)
     */
    static unsigned getAddrSize() {
        return sysInfo().addrSize;
    }

    /**
     * Get the total amount of system memory in MB
     */
    static unsigned long long getMemSizeMB() {
        return sysInfo().memSize / (1024 * 1024);
    }

    /**
     * Get the number of CPUs
     */
    static unsigned getNumCores() {
        return sysInfo().numCores;
    }

    /**
     * Get the number of cores available. Make a best effort to get the cores for this process.
     * If that information is not available, get the total number of CPUs.
     */
    static unsigned long getNumAvailableCores() {
        return ProcessInfo::getNumCoresForProcess().value_or(ProcessInfo::getNumCores());
    }

    /**
     * Get the system page size in bytes.
     */
    static unsigned long long getPageSize() {
        return sysInfo().pageSize;
    }

    /**
     * Get the CPU architecture (e.g. x86, x86_64)
     */
    static const std::string& getArch() {
        return sysInfo().cpuArch;
    }

    /**
     * Determine if NUMA is enabled (interleaved) for this process
     */
    static bool hasNumaEnabled() {
        return sysInfo().hasNuma;
    }

    /**
     * Determine if file zeroing is necessary for newly allocated data files.
     */
    static bool isDataFileZeroingNeeded() {
        return sysInfo().fileZeroNeeded;
    }

    /**
     * Determine if we need to workaround slow msync performance on Illumos/Solaris
     */
    static bool preferMsyncOverFSync() {
        return sysInfo().preferMsyncOverFSync;
    }

    /**
     * Get extra system stats
     */
    void appendSystemDetails(BSONObjBuilder& details) const {
        details.append(StringData("extra"), sysInfo()._extraStats.copy());
    }

    /**
     * Append platform-specific data to obj
     */
    void getExtraInfo(BSONObjBuilder& info);

    bool supported();

    static bool blockCheckSupported();

    static bool blockInMemory(const void* start);

    /**
     * Returns a positive floating point number between 0.0 and 1.0 to inform MMapV1 how much it
     * must remap pages to bring the system page file implementation back below a certain
     * threshold. A number of 1.0 means remap everything.
     */
    static double getSystemMemoryPressurePercentage();

    /**
     * @return a pointer aligned to the start of the page the provided pointer belongs to.
     *
     * NOTE requires blockCheckSupported() == true
     */
    inline static const void* alignToStartOfPage(const void* ptr) {
        return reinterpret_cast<const void*>(reinterpret_cast<unsigned long long>(ptr) &
                                             ~(getPageSize() - 1));
    }

    /**
     * Sets i-th element of 'out' to non-zero if the i-th page starting from the one containing
     * 'start' is in memory.
     * The 'out' vector will be resized to fit the requested number of pages.
     * @return true on success, false otherwise
     *
     * NOTE: requires blockCheckSupported() == true
     */
    static bool pagesInMemory(const void* start, size_t numPages, std::vector<char>* out);

private:
    /**
     * Host and operating system info.  Does not change over time.
     */
    class SystemInfo {
    public:
        std::string osType;
        std::string osName;
        std::string osVersion;
        unsigned addrSize;
        unsigned long long memSize;
        unsigned numCores;
        unsigned long long pageSize;
        std::string cpuArch;
        bool hasNuma;
        BSONObj _extraStats;

        // This is an OS specific value, which determines whether files should be zero-filled
        // at allocation time in order to avoid Microsoft KB 2731284.
        //
        bool fileZeroNeeded;

        // On non-Solaris (ie, Linux, Darwin, *BSD) kernels, prefer msync.
        // Illumos kernels do O(N) scans in memory of the page table during msync which
        // causes high CPU, Oracle Solaris 11.2 and later modified ZFS to workaround mongodb
        // Oracle Solaris Bug:
        //  18658199 Speed up msync() on ZFS by 90000x with this one weird trick
        bool preferMsyncOverFSync;

        SystemInfo()
            : addrSize(0),
              memSize(0),
              numCores(0),
              pageSize(0),
              hasNuma(false),
              fileZeroNeeded(false),
              preferMsyncOverFSync(true) {
            // populate SystemInfo during construction
            collectSystemInfo();
        }

    private:
        /** Collect host system info */
        void collectSystemInfo();
    };

    ProcessId _pid;

    static bool checkNumaEnabled();

    inline static const SystemInfo& sysInfo() {
        static ProcessInfo::SystemInfo systemInfo;
        return systemInfo;
    }

private:
    /**
     * Get the number of available CPUs. Depending on the OS, the number can be the
     * number of available CPUs to the current process or scheduler.
     */
    static boost::optional<unsigned long> getNumCoresForProcess();
};

bool writePidFile(const std::string& path);
}
