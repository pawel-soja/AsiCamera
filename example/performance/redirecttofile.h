#pragma once

#include <cstdio>
#include <string>
#include <unistd.h>

class RedirectToFile
{
    FILE *handle;
public:
    RedirectToFile(int fd, const std::string &fileName)
        : handle(fopen(fileName.data(), "a+"))
    {
        if (handle)
            dup2(fileno(handle), fd);
    }

    ~RedirectToFile()
    {
        if (handle)
            fclose(handle);
    }
};
