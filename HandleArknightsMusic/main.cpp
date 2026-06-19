#include <iostream>
#include <filesystem>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

import WorkModule;

namespace fs = std::filesystem;

int wmain(int argc, wchar_t* argv[]) {
	SetConsoleOutputCP(CP_UTF8);
	setlocale(LC_ALL, ".utf8");

	std::cout << "Info: Cmd line:" << std::endl;
	for (int i = 0; i < argc; ++i) {
		std::wcout << "\t'" << argv[i] << "'" << std::endl;
	}
	std::cout << "End cmd line." << std::endl << std::endl;

#ifndef _DEBUG
	if (argc < 2) {
		cout << "Error: No argument provided." << endl;
		cout << "         Just drag files/folders onto the icon of this app" << endl;
		cout << "         or list paths after app name." << endl;
		system("pause");
		return 0;
	}
#endif // !_DEBUG

	try {
		Work worker;
		if (argc > 1)
			worker.run(argc - 1, argv + 1);
		else
			worker.run(0, nullptr);
	}
	catch (std::exception& excp) {
		std::cout << std::endl << std::endl << "Exception: " << excp.what() << std::endl << std::endl;
		return 1;
	}
	catch (...) {
		std::cout << std::endl << std::endl << "Exception: Unknown." << std::endl << std::endl;
		return 1;
	}
	std::cout << std::endl << "Over." << std::endl << std::endl;
	system("pause");
	return 0;
}
