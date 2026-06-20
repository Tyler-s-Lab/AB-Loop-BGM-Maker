export module ArknightsModule;

import <string>;
import <optional>;
import <algorithm>;

export const std::string COPYRIGHT = "HYPERGRYPH";
export const std::string ORGANIZATION = "ARKNIGHTS";

export struct FilenameRes {
	std::string key; // 去除附加内容的纯粹的音乐名（m_sys_xxx_intro.wav -> m_sys_xxx）
	bool is_intro;
};

export std::optional<FilenameRes> filename2keyword(const std::string& filename) {
	size_t pos; // 承接string::find的返回值
	std::string key;
	bool isIntro = false;

	std::string i = filename;
	std::transform(i.begin(), i.end(), i.begin(), ::tolower); // 转小写，方便匹配

	if (pos = i.find("_intro.wav"); pos != std::string::npos) { // 匹配intro
		// intro和loop都匹配，无法处理
		if (i.find("_loop.wav") != std::string::npos) {
			return {};
		}

		key = filename.substr(0, pos);
		isIntro = true;
	}
	else if (pos = i.find("_loop.wav"); pos != std::string::npos) { // 匹配loop
		key = filename.substr(0, pos);
		isIntro = false;
	}
	else { // intro和loop都无法匹配，无法处理
		return {};
	}
	return { {key, isIntro} };
}
