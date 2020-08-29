#pragma once
#include <ntifs.h>
#include <windef.h>
#include <ntdef.h>
#include <ntimage.h>
#include <intrin.h>
#include <ntddkbd.h>

#include "UsingCPP.hpp"
#include "ErrorCode.hpp"

//BugCheck(WK_UNKNOWN_ERROR, GetExceptionCode(), 0, 0, 0);

#define TRY_START __try \
						{
#define TRY_END(RetStatus) } \
							__except (1) \
							{ \
								PrintErr("[%S] Î´Öª´íÎó! ErrorCode:%X\n", __FUNCTIONW__, GetExceptionCode()); \
							} \
							return RetStatus;

#define TRY_END_NOSTATUS } \
							__except (1) \
							{ \
								PrintErr("[%S] Î´Öª´íÎó! ErrorCode:%X\n", __FUNCTIONW__, GetExceptionCode()); \
							} \
							return;

#ifdef DBG
#define PrintSuc(Format, ...) DbgPrint("[+] SuperDriver: " Format, __VA_ARGS__)
#define PrintIfm(Format, ...) DbgPrint("[*] SuperDriver: " Format, __VA_ARGS__)
#define PrintErr(Format, ...) DbgPrint("[-] SuperDriver: " Format, __VA_ARGS__)

#define BugCheck KeBugCheckEx
#else
#define PrintSuc
#define PrintIfm
#define PrintErr

#define BugCheck
#endif

#pragma warning(disable : 4100)
#pragma warning(disable : 4201)
#pragma warning(disable : 4189)
#pragma warning(disable : 4100)