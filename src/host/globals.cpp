#include "precomp.h"

#include "globals.h"

#pragma hdrstop

CONSOLE_INFORMATION *Globals::getConsoleInformation()
{
    if (!ciConsoleInformation)
    {
        ciConsoleInformation = new CONSOLE_INFORMATION();
    }

    return ciConsoleInformation;
}
