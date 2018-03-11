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

#include <boost/optional.hpp>
#include <cstddef>

namespace mongo {

/**
 *  Provides access to the current stack bounds and remaining
 *  available stack space.
 *
 *  To use one, create it on the stack, like this:
 *
 *  // Construct a new locator
 *  const StackLocator locator;
 *
 *  // Get the start of the stack
 *  auto b = locator.begin();
 *
 *  // Get the end of the stack
 *  auto e = locator.end();
 *
 *  // Get the remaining space after 'locator' on the stack.
 *  auto avail = locator.available();
 */
class StackLocator {
public:
    /**
     * Constructs a new StackLocator. The locator must have automatic
     * storage duration or the behavior is undefined.
     */
    StackLocator();

    /**
     *  Returns the address of the beginning of the stack, or nullptr
     *  if this cannot be done. Beginning here means those addresses
     *  that represent values of automatic duration found earlier in
     *  the call chain. Returns nullptr if the beginning of the stack
     *  could not be found.
     */
    void* begin() const {
        return _begin;
    }

    /**
     *  Returns the address of the end of the stack, or nullptr if
     *  this cannot be done. End here means those addresses that
     *  represent values of automatic duration allocated deeper in the
     *  call chain. Returns nullptr if the end of the stack could not
     *  be found.
     */
    void* end() const {
        return _end;
    }

    /**
     *  Returns the apparent size of the stack. Returns a disengaged
     *  optional if the size of the stack could not be determined.
     */
    boost::optional<size_t> size() const;

    /**
     *  Returns the remaining stack available after the location of
     *  this StackLocator. Obviously, the StackLocator must have been
     *  constructed on the stack. Calling 'available' on a heap
     *  allocated StackAllocator will have undefined behavior. Returns
     *  a disengaged optional if the remaining stack cannot be
     *  determined.
     */
    boost::optional<std::size_t> available() const;

private:
    void* _begin = nullptr;
    void* _end = nullptr;
};

}  // namespace mongo
