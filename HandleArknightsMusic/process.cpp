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

#ifdef HAVE_SFML
#include <SFML/Audio.hpp>
#endif

namespace fs = std::filesystem;
using namespace std;

namespace {

/**
 * @brief 一个音乐对的数据结构
*/
struct PAIRDATA {
	bool hasIntro;
	bool hasLoop;

#ifdef HAVE_SFML
	sf::Int64 offset;
	sf::Int64 length;

	PAIRDATA() :
		hasIntro(false),
		hasLoop(false),
		offset(200000),
		length(0) {}
#else
	PAIRDATA() :
		hasIntro(false),
		hasLoop(false) {}
#endif
};

/**
 * @brief 音乐名对数据结构的map
*/
std::map<std::string, PAIRDATA> g_pairs;


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

	#ifdef HAVE_SFML
		// intro长度即offset
		sf::Music music;
		music.openFromFile(string(".\\InputGameFiles\\") + i);
		tmpdata.offset = music.getDuration().asMicroseconds();
	#endif
	}
	else if ((pos = i.find("_loop.wav")) != string::npos) { // 匹配loop
		name = i.substr(0, pos);
		tmpdata.hasLoop = true;

		cout << "Find: File \'" << i << "\' as Loop \'" << name << "\'." << endl;

	#ifdef HAVE_SFML
		sf::Music music;
		music.openFromFile(string(".\\InputGameFiles\\") + i);
		tmpdata.length = music.getDuration().asMicroseconds();
	#endif
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
		#ifdef HAVE_SFML
			p->second.length = tmpdata.length;
		#endif
		}
		else {
			if (p->second.hasIntro) {
				cout << "Warning: Repeated Intro: \'" << i << "\' as \'" << name << "\'." << endl;
				return;
			}
			p->second.hasIntro = true;
		#ifdef HAVE_SFML
			p->second.offset = tmpdata.offset;
		#endif
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
	fs::path input(".\\InputGameFiles");

	// 检查输入目录
	if (!fs::exists(input) || !fs::is_directory(input)) {
		cout << "Cannot open directory \'InputGameFiles\'!" << endl;
		return false;
	}

	// 遍历输入目录
	for (const fs::directory_entry& it : fs::directory_iterator(input)) {
		if (fs::is_directory(it.status())) {
			cout << "Warning: Find directory in Input: \'" << it.path().string() << "\'." << endl;
		}
		else {
			//cout << "TEST: Get: \'" << it.path().filename().string() << "\'." << endl;
			handleOneFile(it.path().filename().string());
		}
	}
	return true;
}


} // namespace


std::string g_outputpath;


bool process() {
	if (!getFiles()) {
		cout << "Failed to getFiles()!" << endl;
		return false;
	}
	if (g_pairs.empty()) {
		cout << "No file is found" << endl;
		return true;
	}

	fs::create_directory(g_outputpath);
	fs::create_directory(g_outputpath + "\\ogg");
#ifdef HAVE_SFML
#ifdef WRITE_METADATA
	fs::create_directory(g_outputpath + "\\ogg_metadata");
#endif
#endif
	fs::create_directory(g_outputpath + "\\wav");
	fs::create_directory(g_outputpath + "\\done");

	ofstream mylist;
#ifdef HAVE_SFML
	ofstream commentList;
	commentList.open(g_outputpath + "\\comments.txt", ios::out);
	if (!commentList.is_open()) {
		cout << "Warning: Failed to open \'comments.txt\'." << endl;
		return false;
	}
#endif

	for (const pair<string, PAIRDATA>& i : g_pairs) {
		cout << "Process: \'" << i.first << "\'" << endl;

		if (!i.second.hasLoop) {
			cout << "Warning: \'" << i.first << "\' pair has no loop and will be discarded." << endl;
			continue;
		}

		mylist.open("mylist.txt");
		if (!mylist.is_open()) {
			cout << "Error: Failed to open mylist.txt when handling \'" << i.first << "\'." << endl;
			continue;
		}

		if (i.second.hasIntro) {
			mylist << "file .\\\\InputGameFiles\\\\" << i.first << "_intro.wav" << endl;
		}
		else {
			mylist << "file .\\\\core\\\\stempty.wav" << endl;
		}
		mylist << "file .\\\\InputGameFiles\\\\" << i.first << "_loop.wav" << endl;
		mylist << "file .\\\\fadeout.wav" << endl;
	#ifdef HAVE_SFML
		commentList << i.first << ": >" << i.second.offset << ":" << i.second.length << "<. " << endl;
	#endif
		
		//ffmpeg -i i2.wav -to 2.0 -af "afade=t=out:st=0.8:d=1" output.wav
		system((string(FFMPEG_PATH) + " -loglevel warning -y -i \".\\InputGameFiles\\" +
			   i.first +
			   "_loop.wav\" -to 2.0 -af \"afade=t=out:st=0.8:d=1\" fadeout.wav").c_str());

		system((string(FFMPEG_PATH) + " -loglevel warning -y -f concat -safe 0 -i mylist.txt -c:a copy output.wav").c_str());
		system((string(FFMPEG_PATH) + " -loglevel warning -y -i output.wav -af \"afade=t=in:st=0:d=0.001\" -c:a libvorbis output.ogg").c_str());

	#ifdef HAVE_SFML
	#ifdef WRITE_METADATA
		// example
		//ffmpeg -i test.ogg -map 0 -y -codec copy -metadata "DESCRIPTION=xxxx" -metadata "TITLE=xxxname" -metadata "COPYRIGHT=HYPERGRYPH" -metadata "ORGANIZATION=ARKNIGHTS" testoutput.ogg 
		string tmpcmd(FFMPEG_PATH);
		tmpcmd.append(" -loglevel warning -y -i output.ogg -map 0 -codec copy -metadata OHMSSPC=\">");
		tmpcmd.append(to_string(i.second.offset));
		tmpcmd.append(":");
		tmpcmd.append(to_string(i.second.length));
		tmpcmd.append("<\" -metadata TITLE=\"");
		tmpcmd.append(i.first);
		tmpcmd.append("\" -metadata COPYRIGHT=\"HYPERGRYPH\" -metadata ORGANIZATION=\"ARKNIGHTS\" outputmeta.ogg");
		system(tmpcmd.c_str());
	#endif
	#endif

		fs::rename(".\\output.wav", g_outputpath + "\\wav\\" + i.first + ".wav");
		fs::rename(".\\output.ogg", g_outputpath + "\\ogg\\" + i.first + ".ogg");
	#ifdef HAVE_SFML
	#ifdef WRITE_METADATA
		fs::rename(".\\outputmeta.ogg", g_outputpath + "\\ogg_metadata\\" + i.first + ".ogg");
	#endif
	#endif

		if (i.second.hasIntro) {
			fs::rename(".\\InputGameFiles\\" + i.first + "_intro.wav",
					   g_outputpath + "\\done\\" + i.first + "_intro.wav");
		}
		fs::rename(".\\InputGameFiles\\" + i.first + "_loop.wav",
				   g_outputpath + "\\done\\" + i.first + "_loop.wav");

		mylist.close();
	}

	fs::remove(".\\mylist.txt");
	fs::remove(".\\fadeout.wav");

#ifdef HAVE_SFML
	commentList.close();
#endif
	return true;
}
