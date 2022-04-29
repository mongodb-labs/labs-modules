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

#include "mongo/platform/basic.h"

#include "mongo/db/commands.h"
#include "mongo/logv2/log.h"

#include "mongo/db/modules/labs/src/external/registry/endpoint_commands_gen.h"
#include "endpoint.h"
#include "registry.h"

namespace mongo {

namespace {

// Leaked intentionally
Endpoint* getEndpointFromCommand(const RegisterEndpoint& cmd) {

    std::string upperCaseMethod = cmd.getMethod().toString();
    std::string upperCaseRequestEncoding = cmd.getRequestEncoding().toString();
    std::string upperCaseResponseEncoding = cmd.getResponseEncoding().toString();

    std::vector<std::string> httpHeaders;
    auto inputHeaders = cmd.getHttpHeaders();

    for (auto& headerElem : inputHeaders){
        httpHeaders.push_back(headerElem.toString());
    }

    Endpoint* endpoint = new Endpoint(
        cmd.getCommandParameter().toString(),
        cmd.getEndpoint().toString(),
        cmd.getAs().toString(),
        Endpoint::httpMethodFromString(upperCaseMethod),
        httpHeaders,
        Endpoint::encodingTypeFromString(upperCaseRequestEncoding),
        Endpoint::encodingTypeFromString(upperCaseResponseEncoding)
    );

    return endpoint;
}

class RegisterEndpointCommand : public TypedCommand<RegisterEndpointCommand> {
public:
    AllowedOnSecondary secondaryAllowed(ServiceContext*) const override {
        return AllowedOnSecondary::kNever;
    }
    bool adminOnly() const override {
        return true;
    }
    std::string help() const override {
        return "Registers an endpoint to the endpoint registry";
    }

public:
    using Request = RegisterEndpoint;
    using Reply = RegisterEndpointReply;

    class Invocation final : public InvocationBase {
    public:
        using InvocationBase::InvocationBase;

        auto typedRun(OperationContext* opCtx) {

            const auto& cmd = request();

            auto name = cmd.getCommandParameter();

            RegisterEndpointReply reply;

            auto* endpoint = getEndpointFromCommand(cmd);

            bool registerResult = EndpointRegistry::get()->registerEndpoint(
                            name,
                            endpoint);

            reply.setInsert(registerResult);

            return reply;
        }

    private:
        bool supportsWriteConcern() const final {
            return false;
        }

        void doCheckAuthorization(OperationContext*) const final {}

        NamespaceString ns() const override {
            return NamespaceString(request().getDbName(), "");
        }
    };
} RegisterEndpointCommand;

}  // namespace
}  // namespace mongo
