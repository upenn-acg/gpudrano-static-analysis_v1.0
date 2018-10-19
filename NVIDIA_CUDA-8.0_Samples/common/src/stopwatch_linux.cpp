/**
 * Copyright 1993-2013 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

/* Stopwatch Implementation Class for linux */

// includes, file
#include <stopwatch_linux.h>


////////////////////////////////////////////////////////////////////////////////
//! Constructor, default
////////////////////////////////////////////////////////////////////////////////
StopWatchLinux::StopWatchLinux() :
    start_time(),
    diff_time(0.0),
    total_time(0.0),
    running(false),
    clock_sessions(0)
{ }

////////////////////////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////////////////////////
StopWatchLinux::~StopWatchLinux() { }

