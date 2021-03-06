/*
 * Copyright (C) 2015  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under GNU GPL v3, see LICENSE at top level directory.
 * 
 */
#include <stdinc.hpp>
#include "../data_traits.hpp"
using namespace modloader;

struct roadblox_traits
{
    struct dtraits : modloader::dtraits::OpenFile
    {
        static const char* what() { return "roadblocks nodes"; }
    };
};

using OpenRoadBloxDetour = modloader::OpenFileDetour<0x461125, roadblox_traits::dtraits>;

static auto xinit = initializer([](DataPlugin* plugin_ptr)
{
    auto ReloadRoadBlox = injector::cstd<void()>::call<0x461100>;
    plugin_ptr->AddDetour("roadblox.dat", reinstall_since_load, OpenRoadBloxDetour(), gdir_refresh(ReloadRoadBlox));
});
