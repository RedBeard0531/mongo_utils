/** @file admin_access.h
 */

/**
*    Copyright (C) 2010 10gen Inc.
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

#include "mongo/db/auth/user_name.h"
#include "mongo/db/jsobj.h"

namespace mongo {

class OperationContext;

/*
 * An AdminAccess is an interface class used to determine if certain users have
 * privileges to a given resource.
 *
 */
class AdminAccess {
public:
    virtual ~AdminAccess() {}

    /** @return if there are any priviledge users. This should not
     *          block for long and throw if can't get a lock if needed.
     */
    virtual bool haveAdminUsers(OperationContext* opCtx) const = 0;
};

class NoAdminAccess : public AdminAccess {
public:
    virtual ~NoAdminAccess() {}

    virtual bool haveAdminUsers(OperationContext* opCtx) const {
        return false;
    }
};

}  // namespace mongo
