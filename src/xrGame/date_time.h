////////////////////////////////////////////////////////////////////////////
//	Module 		: date_time.h
//	Created 	: 08.05.2004
//  Modified 	: 08.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Date and time routines
////////////////////////////////////////////////////////////////////////////

#pragma once

u64 generate_time(
    u32 years, u32 months, u32 days, u32 hours, u32 minutes, u32 seconds, u32 milliseconds = 0);

void split_time(u64 time,
    u32& years, u32& months, u32& days, u32& hours, u32& minutes, u32& seconds, u32& milliseconds);
