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

	PAIRDATA() :
		hasIntro(false),
		hasLoop(false),
		offset(200000),
		length(0) {}
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
	size_t pos; // 承接string::find的返回值
	string name; // 去除附加内容的纯粹的音乐名（m_sys_xxx_intro.wav -> m_sys_xxx）
	PAIRDATA tmpdata; // 临时音乐对数据

	if ((pos = i.find("_intro.wav")) != string::npos) { // 匹配intro
		// intro和loop都匹配，无法处理
		if (i.find("_loop.wav") != string::npos) {
			cout << "Warning: File \'" << i << "\' is too complex." << endl;
			return;
		}
		// 取音乐名
		name = i.substr(0, pos);
		tmpdata.hasIntro = true;

		cout << "Find: File \'" << i << "\' as Intro \'" << name << "\'." << endl;

		// intro长度即offset
		if (sffile.openFromFile(g_inputpath / i))
			tmpdata.offset = sffile.getSampleCount() - sffile.getSampleCount() % sffile.getChannelCount();
	}
	else if ((pos = i.find("_loop.wav")) != string::npos) { // 匹配loop
		name = i.substr(0, pos);
		tmpdata.hasLoop = true;

		cout << "Find: File \'" << i << "\' as Loop \'" << name << "\'." << endl;

		if (sffile.openFromFile(g_inputpath / i))
			tmpdata.length = sffile.getSampleCount() - sffile.getSampleCount() % sffile.getChannelCount();
	}
	else { // intro和loop都无法匹配，无法处理
		cout << "Warning: File \'" << i << "\' cannot be handled." << endl;
		return;
	}

	map<string, PAIRDATA>::iterator p = g_pairs.find(name);


	if (p == g_pairs.end()) { // 已经有对应文件名的数据对
		g_pairs.emplace(name, tmpdata);
	}
	else {
		if (tmpdata.hasLoop) {
			if (p->second.hasLoop) {
				cout << "Warning: Repeated Loop: \'" << i << "\' as \'" << name << "\'." << endl;
				return;
			}
			p->second.hasLoop = true;
			p->second.length = tmpdata.length;
		}
		else {
			if (p->second.hasIntro) {
				cout << "Warning: Repeated Intro: \'" << i << "\' as \'" << name << "\'." << endl;
				return;
			}
			p->second.hasIntro = true;
			p->second.offset = tmpdata.offset;
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
		cout << "Cannot open directory \'InputGameFiles\'!" << endl;
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


} // namespace


fs::path g_outputpath;
const fs::path g_inputpath = "InputGameFiles";


bool process() {
	bool flac = fs::exists("flac");
	int flacVal = -1;
	if (flac) {
		ifstream flacValFile;
		flacValFile.open("flac");
		if (flacValFile.is_open()) {
			flacValFile >> flacVal;
		}
		flacValFile.close();
	}

	if (!getFiles()) {
		cout << "Failed to getFiles()!" << endl;
		return false;
	}
	if (g_pairs.empty()) {
		cout << "No file is found" << endl;
		return true;
	}

	fs::create_directory(g_outputpath);

	ofstream mylist;

	for (const pair<string, PAIRDATA>& i : g_pairs) {
		cout << "Process: \'" << i.first << "\'" << endl;

		if (!i.second.hasLoop) {
			cout << "!!!! Warning: \'" << i.first << "\' pair has no loop and will be discarded." << endl;
			continue;
		}

		mylist.open("mylist.txt");
		if (!mylist.is_open()) {
			cout << "!!!! Error: Failed to open mylist.txt when handling \'" << i.first << "\'." << endl;
			continue;
		}

		if (i.second.hasIntro) {
			mylist << "file .\\\\InputGameFiles\\\\" << i.first << "_intro.wav" << endl;
		}
		else {
			mylist << "file .\\\\stempty.wav" << endl;
		}
		mylist << "file .\\\\InputGameFiles\\\\" << i.first << "_loop.wav" << endl;
		mylist << "file .\\\\fadeout.wav" << endl;

		//ffmpeg -i i2.wav -to 2.0 -af "afade=t=out:st=0.8:d=1" output.wav
		system((FFMPEG_PATH + " -loglevel warning -y -i \".\\InputGameFiles\\" +
			i.first +
			"_loop.wav\" -to 2.0 -af \"afade=t=out:st=0.8:d=1\" fadeout.wav").c_str());

		// example
		//ffmpeg -i test.ogg -map 0 -y -codec copy -metadata "DESCRIPTION=xxxx" -metadata "TITLE=xxxname" -metadata "COPYRIGHT=HYPERGRYPH" -metadata "ORGANIZATION=ARKNIGHTS" testoutput.ogg 
		string tmpcmd(FFMPEG_PATH);
		tmpcmd.append(" -loglevel warning -y -f concat -safe 0 -i mylist.txt -af \"afade=t=in:st=0:d=0.001\"");
		tmpcmd.append(" -metadata OHMSSPD=\"<");
		tmpcmd.append(to_string(i.second.offset));
		tmpcmd.append("|");
		tmpcmd.append(to_string(i.second.length));
		tmpcmd.append(">\" -metadata TITLE=\"");
		tmpcmd.append(i.first);
		tmpcmd.append("\" -metadata COPYRIGHT=\"HYPERGRYPH\" -metadata ORGANIZATION=\"ARKNIGHTS\"");
		if (flac) {
			tmpcmd.append(" -c:a flac");
			if (flacVal != -1) {
				tmpcmd.append(" -compression_level " + to_string(flacVal));
			}
			tmpcmd.append(" output.flac");
			system(tmpcmd.c_str());
			fs::rename("output.flac", g_outputpath / (i.first + ".flac"));
		}
		else {
			tmpcmd.append(" -c:a libvorbis output.ogg");
			system(tmpcmd.c_str());
			fs::rename("output.ogg", g_outputpath / (i.first + ".ogg"));
		}


		mylist.close();
	}

	fs::remove("mylist.txt");
	fs::remove("fadeout.wav");

	return true;
}
