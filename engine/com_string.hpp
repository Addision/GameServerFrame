#pragma once
#include <string>
#include <list>
namespace Engine
{
	class StringUtil {
	public:
		static void SplitC(const std::string& str, const std::string delim, std::list<std::string>& result);
		static void SplitCpp(const std::string& str, const std::string delim, std::list<std::string>& result);
		// �ж��ַ���str�Ƿ�����prefix��ͷ
		static bool StartsWith(const std::string& str, const std::string& prefix);

		static bool EndsWith(const std::string& str, const std::string& suffix);

		static std::string& Ltrim(std::string& str); // NOLINT

		static std::string& Rtrim(std::string& str); // NOLINT

		static std::string& Trim(std::string& str); // NOLINT

		static void Trim(std::list<std::string>* str_list);

		// �Ӵ��滻
		static void Replace(const std::string& sub_str1,
			const std::string& sub_str2, std::string* str);

		static void UrlEncode(const std::string& src_str, std::string* dst_str);

		static void UrlDecode(const std::string& src_str, std::string* dst_str);

		// ��Сдת��
		static void ToUpper(std::string* str);

		static void ToLower(std::string* str);

		// ȥͷȥβ
		static bool StripSuffix(std::string* str, const std::string& suffix);

		static bool StripPrefix(std::string* str, const std::string& prefix);

		// bin��hexת��
		static bool Hex2Bin(const char* hex_str, std::string* bin_str);

		static bool Bin2Hex(const char* bin_str, std::string* hex_str);
	};

}