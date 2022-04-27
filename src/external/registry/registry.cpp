/**
 *    Copyright (C) 2019-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kDefault

#include "registry.h"
#include "mongo/logv2/log.h"
#include "mongo/util/str.h"

namespace mongo {

bool EndpointRegistry::registerEndpoint(StringData name, Endpoint* endpoint) {
    uassert(40261, str::stream() << "Endpoint names must have at least 1 character", !name.empty());

    auto endpointObj = EndpointRegistry::get()->getEndpoint(name);

    if (endpointObj) {
        _endpoints.erase(name);
        delete endpointObj;
    }

    auto result = _endpoints.try_emplace(name, endpoint);
    return result.second;
}

Endpoint* EndpointRegistry::getEndpoint(StringData name) const {
    auto it = _endpoints.find(name);

    if (it == _endpoints.end()) {
        return nullptr;
    }

    return it->second;
}

EndpointRegistry* EndpointRegistry::get() {
    static auto reg = new EndpointRegistry();
    return reg;
}

}  // namespace mongo