#ifndef _AERO_CORE_ASSERT_H_
#define _AERO_CORE_ASSERT_H_


#ifdef PLATFORM_WINDOWS
	#define DEBUG_BREAK __debugbreak()
#else
	#define DEBUG_BREAK
#endif // PLATFORM_WINDOWS

#ifdef DEBUG
	#define ENABLE_ASSERTS
    #define ENABLE_VERIFY
#endif // DEBUG

#ifdef ENABLE_ASSERTS
	#define ASSERT_MESSAGE_INTERNAL(...)  ::aero::Log::PrintAssertMsg("FAILED-ASSERT", __VA_ARGS__)
	#define ASSERT(condition, ...) { if(!(condition)) { ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); DEBUG_BREAK; } }
#else
	#define ASSERT(condition, ...)
#endif // ENABLE_ASSERTS

#ifdef ENABLE_VERIFY
	#define VERIFY_MESSAGE_INTERNAL(...)  ::aero::Log::PrintAssertMsg("FAILED-VERIFY", __VA_ARGS__)
	#define VERIFY(condition, ...) { if(!(condition)) { VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); } }
#else
	#define VERIFY(condition, fmt, ...)
#endif // ENABLE_VERIFY
#endif // _AERO_CORE_ASSERT_H_
