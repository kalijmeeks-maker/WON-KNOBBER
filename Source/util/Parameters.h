/*
    Parameters.h — central parameter ID constants
    WON-KNOBBER · part of the core layer
*/
#pragma once

namespace ParamIDs
{
    constexpr auto drive = "drive";
    constexpr auto mix = "mix";
    constexpr auto bypass = "bypass"; // host-facing bypass param (registered AFTER drive+mix => index 2)
}
