/**
 *    Copyright (C) 2015 MongoDB Inc.
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

#include <condition_variable>

namespace mongo {
namespace stdx {

using condition_variable = ::std::condition_variable;          // NOLINT
using condition_variable_any = ::std::condition_variable_any;  // NOLINT
using cv_status = ::std::cv_status;                            // NOLINT
using ::std::notify_all_at_thread_exit;                        // NOLINT

}  // namespace stdx
}  // namespace mongo
