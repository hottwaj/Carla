/*
 * Carla JACK Plugin
 * Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaPlugin.hpp"
#include "CarlaEngine.hpp"
#include "CarlaUtils.hpp"

#ifdef BUILD_BRIDGE
# error This file should not be used under bridge mode
#endif

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------

CarlaPlugin* CarlaPlugin::newJackApp(const Initializer& init)
{
    carla_debug("CarlaPlugin::newVST3({%p, \"%s\", \"%s\", " P_INT64 "})", init.engine, init.filename, init.name, init.uniqueId);

#ifdef CARLA_OS_LINUX
    init.engine->setLastError("JACK application support not implemented yet");
    return nullptr;
#else
    init.engine->setLastError("JACK application support not available");
    return nullptr;
#endif
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
