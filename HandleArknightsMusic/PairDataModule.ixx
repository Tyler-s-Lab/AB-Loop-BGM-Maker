export module PairDataModule;

import <string>;
import <filesystem>;

/**
 * @brief 一个音乐对的数据结构
*/
export struct PAIRDATA {
	bool has_intro;
	bool has_loop;

	std::uint64_t offset;
	std::uint64_t length;

	std::filesystem::path intro_filepath;
	std::filesystem::path loop_filepath;

	PAIRDATA() :
		has_intro(false),
		has_loop(false),
		offset(0),
		length(0) {
	}
};
