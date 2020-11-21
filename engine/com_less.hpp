#pragma once
#include <string.h>
namespace Engine
{
	////////////////////////////////////////////////////////////////////////////////
	struct strless
	{
		bool operator()(const char* const& L, const char* const& R) const
		{
			return strcmp(L, R) < 0;
		}
	};
	////////////////////////////////////////////////////////////////////////////////
	struct wstrless
	{
		bool operator()(const wchar_t* const& L, const wchar_t* const& R) const
		{
			return wcscmp(L, R) < 0;
		}
	};
}