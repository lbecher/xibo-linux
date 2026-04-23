#pragma once

#include "control/server/HttpTypes.hpp"

#include "common/fs/FilePath.hpp"

class RestrictiveFileModule
{
public:
    explicit RestrictiveFileModule(FilePath rootDirectory);

    bool matches(const HttpRequest& request) const;
    HttpResponse handle(const HttpRequest& request) const;

private:
    FilePath rootDirectory_;
};
