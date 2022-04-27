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

#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kCommand

#include <boost/algorithm/string.hpp>

#include "mongo/logv2/log.h"
#include "mongo/bson/json.h"
#include "mongo/util/string_map.h"
#include "endpoint.h"

namespace mongo {

/*
* Sends a HTTP Request with a BSONObj in the body if it is a POST request,
* otherwise send without.
*/
BSONObj Endpoint::sendHttpRequest(BSONObj payload) {
    DataBuilder dataBuilder;

    // auto _httpClient = HttpClient::create();
    if (_method == HttpClient::HttpMethod::kGET) {
        auto response = _httpClient->request(_method, _endpoint);
        dataBuilder = std::move(response.body);
    } else {
        // Only support JSON encoding at this time.
        if (_requestEncoding == Endpoint::EncodingType::kJSON) {
            // Create CDR for JSON data
            std::string jsonPayload = payload.jsonString();
            ConstDataRange cdr(jsonPayload.c_str(), jsonPayload.size());
            auto response = _httpClient->request(_method, _endpoint, cdr);
            dataBuilder = std::move(response.body);
        }
    }

    BSONObj obj = parseResponseFromDataBuilder(dataBuilder);

    return obj;
}


// Returns a BSONObj from a given DataBuilder.
BSONObj Endpoint::parseResponseFromDataBuilder(DataBuilder& builder) {
    auto blobSize = builder.size();
    auto blobData = builder.release();

    std::string responseString(blobData.get(), blobData.get() + blobSize);

    BSONObj obj;

    // Only support JSON encoding at this time.
    if (_responseEncoding == Endpoint::EncodingType::kJSON) {
        obj = fromjson(responseString);
    }

    return obj;
}

// For a given encoding type, return the associated enum
Endpoint::EncodingType Endpoint::encodingTypeFromString(std::string encoding) {
    boost::to_upper(encoding);

    // intentionally leaked
    static const auto& encodingTypeAliasMap =
        *new StringMap<Endpoint::EncodingType>{
            {"JSON", Endpoint::EncodingType::kJSON}};

    auto it = encodingTypeAliasMap.find(encoding);
    if (it == encodingTypeAliasMap.end()) {
        return Endpoint::EncodingType::kJSON;
    }

    return it->second;
};

// For a given http method, return the associated enum
HttpClient::HttpMethod Endpoint::httpMethodFromString(std::string method) {
    boost::to_upper(method);

    // intentionally leaked
    static const auto& httpMethodAliasMap =
        *new StringMap<HttpClient::HttpMethod>{
            {"GET", HttpClient::HttpMethod::kGET},
            {"POST", HttpClient::HttpMethod::kPOST},
            {"PUT", HttpClient::HttpMethod::kPUT}};

    auto it = httpMethodAliasMap.find(method);
    if (it == httpMethodAliasMap.end()) {
        return HttpClient::HttpMethod::kGET;
    }

    return it->second;
};

std::string Endpoint::getAs() {
    return _as;
}

} // namespace mongo