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

#pragma once

#include <map>
#include <string>
#include "mongo/util/net/http_client.h"

namespace mongo {

class Endpoint {
public:

    // Encoding types
    enum class EncodingType {
        kJSON
    };

    // Allows mapping from string to EncodingType.
    static EncodingType encodingTypeFromString(std::string name);

    // Allows mapping from string to HttpClient::HttpMethod.
    static HttpClient::HttpMethod httpMethodFromString(std::string name);

    Endpoint(std::string name,
                    std::string endpoint,
                    std::string as,
                    HttpClient::HttpMethod method,
                    std::vector<std::string> httpHeaders,
                    Endpoint::EncodingType requestEncoding,
                    Endpoint::EncodingType responseEncoding)
    : _name(name),
      _endpoint(endpoint),
      _as(as),
      _method(method),
      _httpHeaders(httpHeaders),
      _requestEncoding(requestEncoding),
      _responseEncoding(responseEncoding) {};

    BSONObj sendHttpRequest(BSONObj payload);

    BSONObj parseResponseFromDataBuilder(DataBuilder& builder);

    std::string getAs();

private:
    std::string _name;
    std::string _endpoint;
    std::string _as;
    HttpClient::HttpMethod _method;
    std::vector<std::string> _httpHeaders;
    Endpoint::EncodingType _requestEncoding;
    Endpoint::EncodingType _responseEncoding;

    std::unique_ptr<HttpClient> _httpClient = HttpClient::create();
};

} // namespace mongo