#pragma once
#include <string>
namespace Engine
{
	class NumberFormat
	{
	using byte = unsigned char;
	public:
		static int byte2int(const std::string& bytes)
		{
			if (bytes.size() < 4) return 0;
			int i = 0;
			i = bytes[0] & 0xFF;
			i |= ((bytes[1] << 8) & 0xFF00);
			i |= ((bytes[2] << 16) & 0xFF0000);
			i |= ((bytes[3] << 24) & 0xFF000000);
			return i;
		}
		static void int2byte(int i, std::string& bytes)
		{
			if (bytes.size() < 4) bytes.resize(4);
			bytes[0] = (byte)(0xff & i);
			bytes[1] = (byte)((0xff00 & i) >> 8);
			bytes[2] = (byte)((0xff0000 & i) >> 16);
			bytes[3] = (byte)((0xff000000 & i) >> 24);
		}

	};
}