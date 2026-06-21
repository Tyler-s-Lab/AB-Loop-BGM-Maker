
import <memory>;
import LoggerModule;
import WorkModule;

int wmain(int argc, wchar_t* argv[]) {
	Logger::Init();

	Logger::Info("Cmd line:");
	for (int i = 0; i < argc; ++i) {
		Logger::info << L"    " << argv[i];
	}
	Logger::Info("Cmd end.");
	Console::WriteLine();

	int code = 0;
	switch (0) {
	case 0:
#ifndef _DEBUG
		if (argc < 2) {
			Logger::Error("No argument provided.");
			Logger::Info("     Just drag files/folders onto the icon of this app");
			Logger::Info("     or list paths after app name.");
			code = 1;
			break;
		}
#endif // !_DEBUG

		try {
			auto worker = std::make_unique<Work>();
			if (argc > 1)
				worker->run(argc - 1, argv + 1);
			else
				worker->run(0, nullptr);
		}
		catch (const std::exception& excp) {
			Logger::Exception(excp);
			code = 1;
		}
		catch (...) {
			Logger::Exception("Unknown exception.");
			code = 1;
		}
		break;
	}

	Logger::Pause();
	return code;
}
