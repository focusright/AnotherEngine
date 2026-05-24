#pragma once

class IHostApp;

class HostRunner {
public:
    static int Run(IHostApp& app);
};