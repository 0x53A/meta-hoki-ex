// SPDX-License-Identifier: LGPL-2.1-or-later
#include "locationapibackend.h"

HybrisLocationBackend *getLocationBackend()
{
    return new LocationApiBackend();
}
