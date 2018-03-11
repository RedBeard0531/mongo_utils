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

#include <algorithm>
#include <iomanip>

namespace mongo {

/**
 * A tool to shape data into a table. Input may be any iterable type T of another
 * iterable type U of a string-like type S, for example, a vector of vectors of
 * std::strings. Type S must support an ostream overload and a size() method.
 *
 * Example usage:
 *     std::vector<std::vector<std::string>> rows;
 *
 *     rows.push_back({ "X_VALUE", "Y_VALUE" });
 *     rows.push_back({ "0", "0" });
 *     rows.push_back({ "10.3", "0" });
 *     rows.push_back({ "-0.5", "2" });
 *
 *     std::cout << toTable(rows) << std::endl;
 */
template <typename T>
std::string toTable(const T& rows) {
    const int kDefaultColumnSpacing = 3;
    std::vector<std::size_t> widths;

    for (auto&& row : rows) {
        size_t i = 0;
        for (auto&& value : row) {
            widths.resize(std::max(widths.size(), i + 1));
            widths[i] = std::max(widths[i], value.size());
            i++;
        }
    }

    std::stringstream ss;
    ss << std::left;

    for (auto&& row : rows) {
        size_t i = 0;
        for (auto&& value : row) {
            ss << std::setw(widths[i++] + kDefaultColumnSpacing);
            ss << value;
        }
        ss << "\n";
    }

    return ss.str();
}

}  // namespace mongo
