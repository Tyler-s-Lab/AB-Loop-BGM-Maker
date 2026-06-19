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

namespace fs = std::filesystem;
using namespace std;

int wmain(int argc, wchar_t* argv[]) {
	SetConsoleOutputCP(CP_UTF8);
	setlocale(LC_ALL, ".utf8");
	try {
		cout << "Info: Cmd line:" << endl;
		for (int i = 0; i < argc; ++i) {
			wcout << "\t'" << argv[i] << "'" << endl;
		}
		cout << "End cmd line." << endl << endl;

		// Get file path from command line arguments
#ifdef _DEBUG
		fs::path file_path{ "..\\x64\\test" };
#else
		if (argc < 2) {
			cout << "Error: Unsupported Argument." << endl;
			cout << "Just drag one single folder and drop on the icon of this app." << endl;
			system("pause");
			return 0;
		}
		fs::path file_path{ argv[1] };
#endif // _DEBUG
		g_inputpath = file_path;

		// 生成 带当前时间的 输出目录
		struct tm t = { 0 };
		time_t ts = time(0);
		localtime_s(&t, &ts);
		char buf[128];
		strftime(buf, sizeof(buf), "%Y-%m-%d %H;%M;%S", &t);
		g_outputpath = string("Output ") + buf;

		// 处理
		if (!process()) {
			cout << endl << "Process failed!" << endl << endl;
		}
		else {
			cout << endl << "Over." << endl << endl;
		}

		system("pause");
	}
	catch (std::exception& excp) {
		cout << endl << endl << "Exception: " << excp.what() << endl << endl;
		system("pause");
		return 1;
	}
	catch (...) {
		cout << endl << endl << "Exception: Unknown." << endl << endl;
		system("pause");
		return 1;
	}
	return 0;
}
