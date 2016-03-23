#pragma once

#ifdef __CUDA_ARCH__
/**
 * CUDA knows how to do a proper assert and return its error string to the next cuda* function
 */
#include <assert.h>   
//#define assert(x) if (!(x)) __assertfail((const void *)#x,(const void *)__FILE__,__Line__,(const void *)0,sizeof(unsigned short));

#else 
/**
 * Build our own assert
 */

#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

#if UNIT_TESTING
#include <CppUnitTest.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#include <string> // wstring 
extern void logger(std::string s);
extern std::wstring _errmsg;
#endif


bool assertImpl(bool f, const char* const expr, const char* const file, int line);
#if UNIT_TESTING
#define assert(x) if(!assertImpl((x), #x, __FILE__, __LINE__)) if (!IsDebuggerPresent()) {Assert::IsTrue(false, _errmsg.c_str());} else DebugBreak();
#else // not unit testing
#define assert(x) if(!assertImpl((x), #x, __FILE__, __LINE__)) {DebugBreak();}
#endif
 
#endif // __CUDA_ARCH__

