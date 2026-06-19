/**
*                        HandleArknightsMusic
*  Copyright (c) 2023  Tyler Parret Zhu
*
*                  GNU AFFERO GENERAL PUBLIC LICENSE
*                     Version 3, 19 November 2007
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as published
*  by the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*
*
*  Tyler Parret Zhu (OwlHowlinMornSky) <mysteryworldgod@outlook.com>
*/
#include "process.h"
#include "parse_ini.h"

#include <SFML/Audio/InputSoundFile.hpp>

namespace fs = std::filesystem;
using namespace std;

namespace {

/**
 * @brief 一个音乐对的数据结构
*/
struct PAIRDATA {
	bool hasIntro;
	bool hasLoop;

	std::uint64_t offset;
	std::uint64_t length;

	string introName;
	string loopName;

	PAIRDATA() :
		hasIntro(false),
		hasLoop(false),
		offset(0),
		length(0) {
	}
};

/**
 * @brief 音乐名对数据结构的map
*/
std::map<std::string, PAIRDATA> g_pairs;
sf::InputSoundFile sffile;

/**
 * @brief 处理获取的一个文件名
 * @param i 文件名
*/
void handleOneFile(const string& i) {
	string name; // 去除附加内容的纯粹的音乐名（m_sys_xxx_intro.wav -> m_sys_xxx）
	PAIRDATA tmpdata; // 临时音乐对数据

	if (auto res = base::filename2keyword(i); !res) {
		cerr << "Warning: File \'" << i << "\' cannot be handled." << endl;
		return;
	}
	else {
		name = res.value().first;
		if (res.value().second) { // 匹配intro
			tmpdata.hasIntro = true;

			cout << "Find: File \'" << i << "\' as Intro of \'" << name << "\'." << endl;

			// intro长度即offset
			if (sffile.openFromFile(g_inputpath / i))
				tmpdata.offset = sffile.getSampleCount() - sffile.getSampleCount() % sffile.getChannelCount();

			tmpdata.introName = i;
		}
		else { // 匹配loop
			tmpdata.hasLoop = true;

			cout << "Find: File \'" << i << "\' as Loop of \'" << name << "\'." << endl;

			if (sffile.openFromFile(g_inputpath / i))
				tmpdata.length = sffile.getSampleCount() - sffile.getSampleCount() % sffile.getChannelCount();

			tmpdata.loopName = i;
		}
	}

	map<string, PAIRDATA>::iterator p = g_pairs.find(name);

	if (p == g_pairs.end()) {
		g_pairs.emplace(name, tmpdata);
	}
	else { // 已经有对应文件名的数据对
		if (tmpdata.hasLoop) {
			if (p->second.hasLoop) {
				cout << "Warning: Repeated Loop: \'" << i << "\' as \'" << name << "\'." << endl;
				return;
			}
			p->second.hasLoop = true;
			p->second.length = tmpdata.length;
			p->second.loopName = i;
		}
		else {
			if (p->second.hasIntro) {
				cout << "Warning: Repeated Intro: \'" << i << "\' as \'" << name << "\'." << endl;
				return;
			}
			p->second.hasIntro = true;
			p->second.offset = tmpdata.offset;
			p->second.introName = i;
		}
	}
	return;
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
		cout << "Cannot open directory \"" << input << "\"!" << endl;
		return false;
	}

	// 遍历输入目录
	for (const fs::directory_entry& it : fs::directory_iterator(input)) {
		if (fs::is_directory(it.status())) {
			cout << "Warning: Find directory in Input: \'" << it.path() << "\'." << endl;
		}
		else {
			//cout << "TEST: Get: \'" << it.path().filename().string() << "\'." << endl;
			handleOneFile(it.path().filename().string());
		}
	}
	return true;
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

std::shared_ptr<ini::IniConfig> g_iniConfig;
void read_ini() {
	fs::path ini_path = ".";
	ini_path /= name_of_this_app + ".ini";

	if (!fs::exists(ini_path)) {
		cerr << "Warning: Ini file missing." << endl;
		return;
	}

	if (auto res = ini::read_ini_file(ini_path.string()); !res) {
		cerr << "Warning: Ini file corrupted." << endl;
		return;
	}
	else {
		g_iniConfig = res.value();
	}
}

} // namespace


fs::path g_outputpath;
fs::path g_inputpath;


bool process() {
	///////////////////////////////////////////////////////
	read_ini();

	bool set_title = false;
	bool set_copyright = false;
	bool set_organization = false;
	bool flac = false;
	int flacVal = -1;
	int oggAq = -99;
	if (g_iniConfig) {
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

		if (auto p = std::get_if<string>(&config["format"]); p) {
			if (*p == "flac") {
				flac = true;

				if (auto pp = std::get_if<int64_t>(&config["flac_compression_level"]); pp) {
					if (*pp < 0 || *pp > 8) {
						cerr << "Warning: Invalid flac_compression_level in ini file, ignored." << endl;
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
						cerr << "Warning: Invalid ogg_quality in ini file, ignored." << endl;
					}
					else {
						oggAq = static_cast<int>(*pp);
					}
				}
			}
			else {
				cerr << "Warning: Invalid format in ini file, defaulting to ogg." << endl;
			}
		}
	}

	///////////////////////////////////////////////////////
	if (!getFiles()) {
		cout << "Failed to getFiles()!" << endl;
		return false;
	}
	if (g_pairs.empty()) {
		cout << "No file is found" << endl;
		return true;
	}
	fs::create_directory(g_outputpath);

	uint64_t empty_offset = 0;
	if (!sffile.openFromFile("empty.wav")) {
		cout << "Failed to read size of empty.wav!" << endl;
		return false;
	}
	empty_offset = sffile.getSampleCount() - sffile.getSampleCount() % sffile.getChannelCount();

	ofstream mylist;

	for (const pair<string, PAIRDATA>& i : g_pairs) {
		cout << "Process: \'" << i.first << "\'" << endl;

		const string& key = i.first;
		const PAIRDATA& data = i.second;

		if (!data.hasLoop) {
			cout << "!!!! Warning: \'" << key << "\' pair has no loop, skipping." << endl;
			continue;
		}

		mylist.open("mylist.txt");
		if (!mylist.is_open()) {
			cout << "!!!! Error: Failed to open mylist.txt when handling \'" << key << "\'." << endl;
			continue;
		}

		uint64_t offset = 0;
		if (data.hasIntro) {
			mylist << "file \'" << doubleSlashes((g_inputpath / data.introName).string()) << '\'' << endl;
			offset = data.offset;
		}
		else {
			mylist << "file .\\\\empty.wav" << endl;
			offset = empty_offset;
		}
		mylist << "file \'" << doubleSlashes((g_inputpath / data.loopName).string()) << '\'' << endl;
		mylist << "file .\\\\fadeout.wav" << endl;

		//ffmpeg -i i2.wav -to 2.0 -af "afade=t=out:st=0.8:d=1" output.wav
		system(
			(FFMPEG_PATH + " -loglevel warning -y -i \"" +
				(g_inputpath / data.loopName).string() +
				"\" -to 2.0 -af \"afade=t=out:st=0.8:d=1\" fadeout.wav").c_str()
		);

		// example
		//ffmpeg -i test.ogg -map 0 -y -codec copy -metadata "DESCRIPTION=xxxx" -metadata "TITLE=xxxname" -metadata "COPYRIGHT=HYPERGRYPH" -metadata "ORGANIZATION=ARKNIGHTS" testoutput.ogg 
		string tmpcmd(FFMPEG_PATH);
		tmpcmd.append(" -loglevel warning -y -f concat -safe 0 -i mylist.txt -af \"afade=t=in:st=0:d=0.001\"");
		tmpcmd.append(" -metadata OHMSSPD=\"<");
		tmpcmd.append(to_string(offset) + "|");
		tmpcmd.append(to_string(data.length) + ">\"");
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
				tmpcmd.append(" -compression_level " + to_string(flacVal));
			}
			tmpcmd.append(" output.flac");

			system(tmpcmd.c_str());
			fs::rename("output.flac", g_outputpath / (key + ".flac"));
		}
		else {
			tmpcmd.append(" -c:a libvorbis");
			if (oggAq != -99) {
				tmpcmd.append(" -aq " + to_string(oggAq));
			}
			tmpcmd.append(" output.ogg");

			system(tmpcmd.c_str());
			fs::rename("output.ogg", g_outputpath / (key + ".ogg"));
		}

		mylist.close();
	}

	fs::remove("mylist.txt");
	fs::remove("fadeout.wav");

	return true;
}

