#pragma once

#include "../gd.h"
#include "../dav.h"

#define JKSV_DRIVE_FOLDER "JKSV"

namespace fs
{
    extern drive::gd *gDrive;
    extern drive::dav *davDrive;
    extern std::string jksvDriveID;

    void driveInit();
    void driveExit();
    std::string driveSignInGetAuthCode();
}