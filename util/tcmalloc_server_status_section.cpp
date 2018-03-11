/*    Copyright 2013 10gen Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault

#ifdef _WIN32
#define NVALGRIND
#endif

#include "mongo/platform/basic.h"

#include <gperftools/malloc_extension.h>
#include <valgrind/valgrind.h>

#include "mongo/base/init.h"
#include "mongo/db/commands/server_status.h"
#include "mongo/db/server_parameters.h"
#include "mongo/db/service_context.h"
#include "mongo/transport/service_entry_point.h"
#include "mongo/transport/thread_idle_callback.h"
#include "mongo/util/log.h"
#include "mongo/util/net/listen.h"

namespace mongo {

namespace {
// If many clients are used, the per-thread caches become smaller and chances of
// rebalancing of free space during critical sections increases. In such situations,
// it is better to release memory when it is likely the thread will be blocked for
// a long time.
const int kManyClients = 40;

stdx::mutex tcmallocCleanupLock;

MONGO_EXPORT_SERVER_PARAMETER(tcmallocEnableMarkThreadTemporarilyIdle, bool, false);

/**
 *  Callback to allow TCMalloc to release freed memory to the central list at
 *  favorable times. Ideally would do some milder cleanup or scavenge...
 */
void threadStateChange() {

    if (!tcmallocEnableMarkThreadTemporarilyIdle.load()) {
        return;
    }

    if (getGlobalServiceContext()->getServiceEntryPoint()->numOpenSessions() <= kManyClients)
        return;

#if MONGO_HAVE_GPERFTOOLS_GET_THREAD_CACHE_SIZE
    size_t threadCacheSizeBytes = MallocExtension::instance()->GetThreadCacheSize();

    static const size_t kMaxThreadCacheSizeBytes = 0x10000;
    if (threadCacheSizeBytes < kMaxThreadCacheSizeBytes) {
        // This number was chosen a bit magically.
        // At 1000 threads and the current (64mb) thread local cache size, we're "full".
        // So we may want this number to scale with the number of current clients.
        return;
    }

    LOG(1) << "thread over memory limit, cleaning up, current: " << (threadCacheSizeBytes / 1024)
           << "k";

    // We synchronize as the tcmalloc central list uses a spinlock, and we can cause a really
    // terrible runaway if we're not careful.
    stdx::lock_guard<stdx::mutex> lk(tcmallocCleanupLock);
#endif
    MallocExtension::instance()->MarkThreadTemporarilyIdle();
}

// Register threadStateChange callback
MONGO_INITIALIZER(TCMallocThreadIdleListener)(InitializerContext*) {
    if (!RUNNING_ON_VALGRIND)
        registerThreadIdleCallback(&threadStateChange);
    return Status::OK();
}

class TCMallocServerStatusSection : public ServerStatusSection {
public:
    TCMallocServerStatusSection() : ServerStatusSection("tcmalloc") {}
    virtual bool includeByDefault() const {
        return true;
    }

    virtual BSONObj generateSection(OperationContext* opCtx,
                                    const BSONElement& configElement) const {
        long long verbosity = 1;
        if (configElement) {
            // Relies on the fact that safeNumberLong turns non-numbers into 0.
            long long configValue = configElement.safeNumberLong();
            if (configValue) {
                verbosity = configValue;
            }
        }

        BSONObjBuilder builder;

        // For a list of properties see the "Generic Tcmalloc Status" section of
        // http://google-perftools.googlecode.com/svn/trunk/doc/tcmalloc.html and
        // http://code.google.com/p/gperftools/source/browse/src/gperftools/malloc_extension.h
        {
            BSONObjBuilder sub(builder.subobjStart("generic"));
            appendNumericPropertyIfAvailable(
                sub, "current_allocated_bytes", "generic.current_allocated_bytes");
            appendNumericPropertyIfAvailable(sub, "heap_size", "generic.heap_size");
        }
        {
            BSONObjBuilder sub(builder.subobjStart("tcmalloc"));

            appendNumericPropertyIfAvailable(
                sub, "pageheap_free_bytes", "tcmalloc.pageheap_free_bytes");
            appendNumericPropertyIfAvailable(
                sub, "pageheap_unmapped_bytes", "tcmalloc.pageheap_unmapped_bytes");
            appendNumericPropertyIfAvailable(
                sub, "max_total_thread_cache_bytes", "tcmalloc.max_total_thread_cache_bytes");
            appendNumericPropertyIfAvailable(sub,
                                             "current_total_thread_cache_bytes",
                                             "tcmalloc.current_total_thread_cache_bytes");
            // Not including tcmalloc.slack_bytes since it is deprecated.

            // Calculate total free bytes, *excluding the page heap*
            size_t central;
            size_t transfer;
            size_t thread;
            if (MallocExtension::instance()->GetNumericProperty("tcmalloc.central_cache_free_bytes",
                                                                &central) &&
                MallocExtension::instance()->GetNumericProperty(
                    "tcmalloc.transfer_cache_free_bytes", &transfer) &&
                MallocExtension::instance()->GetNumericProperty("tcmalloc.thread_cache_free_bytes",
                                                                &thread)) {
                sub.appendNumber("total_free_bytes", central + transfer + thread);
            }
            appendNumericPropertyIfAvailable(
                sub, "central_cache_free_bytes", "tcmalloc.central_cache_free_bytes");
            appendNumericPropertyIfAvailable(
                sub, "transfer_cache_free_bytes", "tcmalloc.transfer_cache_free_bytes");
            appendNumericPropertyIfAvailable(
                sub, "thread_cache_free_bytes", "tcmalloc.thread_cache_free_bytes");
            appendNumericPropertyIfAvailable(
                sub, "aggressive_memory_decommit", "tcmalloc.aggressive_memory_decommit");

            appendNumericPropertyIfAvailable(
                sub, "pageheap_committed_bytes", "tcmalloc.pageheap_committed_bytes");
            appendNumericPropertyIfAvailable(
                sub, "pageheap_scavenge_count", "tcmalloc.pageheap_scavenge_count");
            appendNumericPropertyIfAvailable(
                sub, "pageheap_commit_count", "tcmalloc.pageheap_commit_count");
            appendNumericPropertyIfAvailable(
                sub, "pageheap_total_commit_bytes", "tcmalloc.pageheap_total_commit_bytes");
            appendNumericPropertyIfAvailable(
                sub, "pageheap_decommit_count", "tcmalloc.pageheap_decommit_count");
            appendNumericPropertyIfAvailable(
                sub, "pageheap_total_decommit_bytes", "tcmalloc.pageheap_total_decommit_bytes");
            appendNumericPropertyIfAvailable(
                sub, "pageheap_reserve_count", "tcmalloc.pageheap_reserve_count");
            appendNumericPropertyIfAvailable(
                sub, "pageheap_total_reserve_bytes", "tcmalloc.pageheap_total_reserve_bytes");
            appendNumericPropertyIfAvailable(
                sub, "spinlock_total_delay_ns", "tcmalloc.spinlock_total_delay_ns");

#if MONGO_HAVE_GPERFTOOLS_SIZE_CLASS_STATS
            if (verbosity >= 2) {
                // Size class information
                BSONArrayBuilder arr;
                MallocExtension::instance()->SizeClasses(&arr, appendSizeClassInfo);
                sub.append("size_classes", arr.arr());
            }
#endif

            char buffer[4096];
            MallocExtension::instance()->GetStats(buffer, sizeof buffer);
            builder.append("formattedString", buffer);
        }

        return builder.obj();
    }

private:
    static void appendNumericPropertyIfAvailable(BSONObjBuilder& builder,
                                                 StringData bsonName,
                                                 const char* property) {
        size_t value;
        if (MallocExtension::instance()->GetNumericProperty(property, &value))
            builder.appendNumber(bsonName, value);
    }

#if MONGO_HAVE_GPERFTOOLS_SIZE_CLASS_STATS
    static void appendSizeClassInfo(void* bsonarr_builder, const base::MallocSizeClass* stats) {
        BSONArrayBuilder* builder = reinterpret_cast<BSONArrayBuilder*>(bsonarr_builder);
        BSONObjBuilder doc;

        doc.appendNumber("bytes_per_object", stats->bytes_per_obj);
        doc.appendNumber("pages_per_span", stats->pages_per_span);
        doc.appendNumber("num_spans", stats->num_spans);
        doc.appendNumber("num_thread_objs", stats->num_thread_objs);
        doc.appendNumber("num_central_objs", stats->num_central_objs);
        doc.appendNumber("num_transfer_objs", stats->num_transfer_objs);
        doc.appendNumber("free_bytes", stats->free_bytes);
        doc.appendNumber("allocated_bytes", stats->alloc_bytes);

        builder->append(doc.obj());
    }
#endif
} tcmallocServerStatusSection;
}
}
