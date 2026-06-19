export module PairDataModule;

import <string>;

/**
 * @brief 一个音乐对的数据结构
*/
export struct PAIRDATA {
	bool hasIntro;
	bool hasLoop;

	std::uint64_t offset;
	std::uint64_t length;

	std::string introName;
	std::string loopName;

	PAIRDATA() :
		hasIntro(false),
		hasLoop(false),
		offset(0),
		length(0) {
	}
};
