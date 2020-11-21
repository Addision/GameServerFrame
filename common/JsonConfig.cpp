#include "JsonConfig.hpp"

bool JsonConfig::Load(const char* json_file)
{
	Json::Reader fileReader;
	std::ifstream ifs(json_file);
	if (ifs.good())
	{
		std::string str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		if (fileReader.parse(str, m_root, true))
		{
			ifs.close();
			return true;
		}
	}
	ifs.close();
	return false;
}