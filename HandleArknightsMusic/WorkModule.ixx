export module WorkModule;

import <map>;
import <string>;
import <fstream>;
import <iostream>;
import <filesystem>;
import <SFML/Audio/InputSoundFile.hpp>;
import "parse_ini.h";

import PairDataModule;
import ArknightsModule;

namespace fs = std::filesystem;

export class Work {
	const std::string FFMPEG_PATH = "ffmpeg";

	fs::path g_outputpath;
	fs::path g_inputpath;

	bool set_title = false;
	bool set_copyright = false;
	bool set_organization = false;
	bool flac = false;
	int flacVal = -1;
	int oggAq = -99;

	std::map<std::string, PAIRDATA> g_pairs; // 音乐名对数据结构的map
	sf::InputSoundFile sffile;

public:
	Work() = default;

	void run(int argc, wchar_t* argv[]) {
		
		// 生成 带当前时间的 输出目录
		struct tm t = { 0 };
		time_t ts = time(0);
		localtime_s(&t, &ts);
		char buf[128];
		strftime(buf, sizeof(buf), "%Y-%m-%d %H;%M;%S", &t);
		g_outputpath = std::string("Output ") + buf;

		// Get file path from command line arguments
		fs::path file_path{ argv[1] };
		g_inputpath = file_path;

		read_ini();

		if (!getFiles()) {
			throw std::exception("Failed to getFiles()!");
		}
		if (g_pairs.empty()) {
			throw std::exception("No file is found");
		}
		fs::create_directory(g_outputpath);

		uint64_t empty_offset = 0;
		if (!sffile.openFromFile("empty.wav")) {
			throw std::exception("Failed to read size of empty.wav!");
		}
		empty_offset = sffile.getSampleCount() - sffile.getSampleCount() % sffile.getChannelCount();

		std::ofstream mylist;

		for (const std::pair<std::string, PAIRDATA>& i : g_pairs) {
			std::cout << "Process: \'" << i.first << "\'" << std::endl;

			const std::string& key = i.first;
			const PAIRDATA& data = i.second;

			if (!data.hasLoop) {
				std::cout << "!!!! Warning: \'" << key << "\' pair has no loop, skipping." << std::endl;
				continue;
			}

			mylist.open("mylist.txt");
			if (!mylist.is_open()) {
				std::cout << "!!!! Error: Failed to open mylist.txt when handling \'" << key << "\'." << std::endl;
				continue;
			}

			uint64_t offset = 0;
			if (data.hasIntro) {
				mylist << "file \'" << doubleSlashes((g_inputpath / data.introName).string()) << '\'' << std::endl;
				offset = data.offset;
			}
			else {
				mylist << "file .\\\\empty.wav" << std::endl;
				offset = empty_offset;
			}
			mylist << "file \'" << doubleSlashes((g_inputpath / data.loopName).string()) << '\'' << std::endl;
			mylist << "file .\\\\fadeout.wav" << std::endl;

			//ffmpeg -i i2.wav -to 2.0 -af "afade=t=out:st=0.8:d=1" output.wav
			system(
				(FFMPEG_PATH + " -loglevel warning -y -i \"" +
					(g_inputpath / data.loopName).string() +
					"\" -to 2.0 -af \"afade=t=out:st=0.8:d=1\" fadeout.wav").c_str()
			);

			// example
			//ffmpeg -i test.ogg -map 0 -y -codec copy -metadata "DESCRIPTION=xxxx" -metadata "TITLE=xxxname" -metadata "COPYRIGHT=HYPERGRYPH" -metadata "ORGANIZATION=ARKNIGHTS" testoutput.ogg 
			std::string tmpcmd(FFMPEG_PATH);
			tmpcmd.append(" -loglevel warning -y -f concat -safe 0 -i mylist.txt -af \"afade=t=in:st=0:d=0.001\"");
			tmpcmd.append(" -metadata OHMSSPD=\"<");
			tmpcmd.append(std::to_string(offset) + "|");
			tmpcmd.append(std::to_string(data.length) + ">\"");
			if (set_title) {
				tmpcmd.append(" -metadata TITLE=\"");
				tmpcmd.append(key + "\"");
			}
			if (set_copyright) {
				tmpcmd.append(" -metadata COPYRIGHT=\"");
				tmpcmd.append(COPYRIGHT + "\"");
			}
			if (set_organization) {
				tmpcmd.append(" -metadata ORGANIZATION=\"");
				tmpcmd.append(ORGANIZATION + "\"");
			}

			if (flac) {
				tmpcmd.append(" -c:a flac");
				if (flacVal != -1) {
					tmpcmd.append(" -compression_level " + std::to_string(flacVal));
				}
				tmpcmd.append(" output.flac");

				system(tmpcmd.c_str());
				fs::rename("output.flac", g_outputpath / (key + ".flac"));
			}
			else {
				tmpcmd.append(" -c:a libvorbis");
				if (oggAq != -99) {
					tmpcmd.append(" -aq " + std::to_string(oggAq));
				}
				tmpcmd.append(" output.ogg");

				system(tmpcmd.c_str());
				fs::rename("output.ogg", g_outputpath / (key + ".ogg"));
			}

			mylist.close();
		}

		fs::remove("mylist.txt");
		fs::remove("fadeout.wav");

		return;
	}

	~Work() = default;

private:

	void read_ini() {
		fs::path ini_path = ".";
		ini_path /= name_of_this_app + ".ini";

		if (!fs::exists(ini_path)) {
			std::cerr << "Warning: Ini file missing." << std::endl;
			return;
		}

		std::shared_ptr<ini::IniConfig> g_iniConfig;
		if (auto res = ini::read_ini_file(ini_path.string()); !res) {
			std::cerr << "Warning: Ini file corrupted." << std::endl;
			return;
		}
		else {
			g_iniConfig = res.value();

			ini::IniConfig& config = *g_iniConfig;

			if (auto p = std::get_if<int64_t>(&config["set_title"]); p) {
				set_title = (*p != 0);
			}
			if (auto p = std::get_if<int64_t>(&config["set_copyright"]); p) {
				set_copyright = (*p != 0);
			}
			if (auto p = std::get_if<int64_t>(&config["set_organization"]); p) {
				set_organization = (*p != 0);
			}

			if (auto p = std::get_if<std::string>(&config["format"]); p) {
				if (*p == "flac") {
					flac = true;

					if (auto pp = std::get_if<int64_t>(&config["flac_compression_level"]); pp) {
						if (*pp < 0 || *pp > 8) {
							std::cerr << "Warning: Invalid flac_compression_level in ini file, ignored." << std::endl;
						}
						else {
							flacVal = static_cast<int>(*pp);
						}
					}
				}
				else if (*p == "ogg") {
					flac = false;

					if (auto pp = std::get_if<int64_t>(&config["ogg_quality"]); pp) {
						if (*pp < 0 || *pp > 10) {
							std::cerr << "Warning: Invalid ogg_quality in ini file, ignored." << std::endl;
						}
						else {
							oggAq = static_cast<int>(*pp);
						}
					}
				}
				else {
					std::cerr << "Warning: Invalid format in ini file, defaulting to ogg." << std::endl;
				}
			}
		}
	}

	/**
	 * @brief 遍历输入目录下的所有文件
	 * @return 函数是否成功
	*/
	bool getFiles() {
		g_pairs.clear();

		// 打开输入目录
		fs::path input(g_inputpath);

		// 检查输入目录
		if (!fs::exists(input) || !fs::is_directory(input)) {
			std::cout << "Cannot open directory \"" << input << "\"!" << std::endl;
			return false;
		}

		// 遍历输入目录
		for (const fs::directory_entry& it : fs::directory_iterator(input)) {
			if (fs::is_directory(it.status())) {
				std::cout << "Warning: Find directory in Input: \'" << it.path() << "\'." << std::endl;
			}
			else {
				//cout << "TEST: Get: \'" << it.path().filename().string() << "\'." << endl;
				handleOneFile(it.path().filename().string());
			}
		}
		return true;
	}

	/**
	 * @brief 处理获取的一个文件名
	 * @param i 文件名
	*/
	void handleOneFile(const std::string& i) {
		std::string name; // 去除附加内容的纯粹的音乐名（m_sys_xxx_intro.wav -> m_sys_xxx）
		PAIRDATA tmpdata; // 临时音乐对数据

		if (auto res = filename2keyword(i); !res) {
			std::cerr << "Warning: File \'" << i << "\' cannot be handled." << std::endl;
			return;
		}
		else {
			name = res.value().first;
			if (res.value().second) { // 匹配intro
				tmpdata.hasIntro = true;

				std::cout << "Find: File \'" << i << "\' as Intro of \'" << name << "\'." << std::endl;

				// intro长度即offset
				if (sffile.openFromFile(g_inputpath / i))
					tmpdata.offset = sffile.getSampleCount() - sffile.getSampleCount() % sffile.getChannelCount();

				tmpdata.introName = i;
			}
			else { // 匹配loop
				tmpdata.hasLoop = true;

				std::cout << "Find: File \'" << i << "\' as Loop of \'" << name << "\'." << std::endl;

				if (sffile.openFromFile(g_inputpath / i))
					tmpdata.length = sffile.getSampleCount() - sffile.getSampleCount() % sffile.getChannelCount();

				tmpdata.loopName = i;
			}
		}

		std::map<std::string, PAIRDATA>::iterator p = g_pairs.find(name);

		if (p == g_pairs.end()) {
			g_pairs.emplace(name, tmpdata);
		}
		else { // 已经有对应文件名的数据对
			if (tmpdata.hasLoop) {
				if (p->second.hasLoop) {
					std::cout << "Warning: Repeated Loop: \'" << i << "\' as \'" << name << "\'." << std::endl;
					return;
				}
				p->second.hasLoop = true;
				p->second.length = tmpdata.length;
				p->second.loopName = i;
			}
			else {
				if (p->second.hasIntro) {
					std::cout << "Warning: Repeated Intro: \'" << i << "\' as \'" << name << "\'." << std::endl;
					return;
				}
				p->second.hasIntro = true;
				p->second.offset = tmpdata.offset;
				p->second.introName = i;
			}
		}
		return;
	}

	std::string doubleSlashes(const std::string& input) {
		std::string result;
		result.reserve(input.size() * 2); // 预分配空间，避免多次重分配
		for (char ch : input) {
			if (ch == '\\' || ch == '/') {
				result.push_back(ch);
				result.push_back(ch); // 重复两次
			}
			else {
				result.push_back(ch);
			}
		}
		return result;
	}

};
