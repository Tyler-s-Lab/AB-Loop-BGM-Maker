export module WorkModule;

import <map>;
import <chrono>;
import <format>;
import <string>;
import <fstream>;
import <filesystem>;
import <SFML/Audio/InputSoundFile.hpp>;

import TempGuardModule;
import IniParserModule;
import LoggerModule;
import CmdArgsHelperModule;
import WinHelperModule;

import PairDataModule;
import ArknightsModule;

namespace fs = std::filesystem;

export class Work {
	const size_t Max_Recurse_Depth = 8;
	const std::string FFMPEG_PATH = "ffmpeg.exe";

	sf::InputSoundFile _sffile;
	uint64_t offset_of_empty_intro = 0;

	fs::path output_dir_path;

	bool set_title = false;
	bool set_copyright = false;
	bool set_organization = false;
	bool output_flac_not_ogg = false;
	int output_flac_compression_level = -1;
	int output_ogg_audio_quality = -99;

	std::map<std::wstring, PAIRDATA> g_pairs; // 音乐名对数据结构的map

public:
	Work() = default;

	void run(int argc, wchar_t* argv[]) {
		read_ini();

		if (!_sffile.openFromFile("empty.wav")) {
			throw std::exception("Failed to read size of empty.wav!");
		}
		offset_of_empty_intro = _sffile.getSampleCount() - _sffile.getSampleCount() % _sffile.getChannelCount();

#ifdef _DEBUG
		if (argc < 1) {
			enum_one_item(fs::path{ LR"(C:\Users\Myste\protable_apps\.OHMS\BGM-Maker Release\test)" });
		}
#endif // _DEBUG
		for (int i = 0; i < argc; ++i) {
			enum_one_item(fs::path{ argv[i] });
		}

		if (g_pairs.empty()) {
			throw std::exception("No file was found.");
		}

		generate_output_path();

		process_pairs();
		return;
	}

	~Work() = default;

private:
	// 生成 带当前时间的 输出目录
	void generate_output_path() {
		std::chrono::sys_seconds time = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
		output_dir_path = std::format("Output {0:%Y-%m-%d %H;%M;%S}", time);
		return;
	}

	void read_ini() {
		fs::path ini_path = ".";
		ini_path /= "config.ini";

		if (!fs::exists(ini_path)) {
			Logger::warning << "Config file missing: " << ini_path << ".";
			return;
		}

		std::shared_ptr<ini::IniConfig> g_iniConfig;
		if (auto res = ini::read_ini_file(ini_path.string()); !res) {
			Logger::warning << "Config file corrupted: " << ini_path << ".";
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
					output_flac_not_ogg = true;

					if (auto pp = std::get_if<int64_t>(&config["flac_compression_level"]); pp) {
						if (*pp < 0 || *pp > 8) {
							Logger::warning << "Invalid flac_compression_level (0 - 8) in ini file, ignored: '" << *pp << "'.";
						}
						else {
							output_flac_compression_level = static_cast<int>(*pp);
						}
					}
				}
				else if (*p == "ogg") {
					output_flac_not_ogg = false;

					if (auto pp = std::get_if<int64_t>(&config["ogg_quality"]); pp) {
						if (*pp < 0 || *pp > 10) {
							Logger::warning << "Invalid ogg_quality (0 - 10) in ini file, ignored: '" << *pp << "'.";
						}
						else {
							output_ogg_audio_quality = static_cast<int>(*pp);
						}
					}
				}
				else {
					Logger::warning << "Invalid format in ini file, defaulting to ogg: '" << *p << "'.";
				}
			}
		}
	}

	void enum_one_item(fs::path input) {
		static size_t depth = 0;
		if (depth >= Max_Recurse_Depth) {
			Logger::warning << "Reached Max_Recurse_Depth (" << Max_Recurse_Depth << "), skipping: " << input << ".";
			return;
		}
		TempGuard tg{ depth };
		depth++;

		if (fs::is_directory(input)) {
			for (const fs::directory_entry& it : fs::directory_iterator(input)) {
				enum_one_item(it.path());
			}
		}
		else if (fs::is_regular_file(input)) {
			enum_one_file(input);
		}
		else {
			Logger::warning << "Unknown item, skipping: " << input << ".";
		}

		depth--;
		return;
	}

	void enum_one_file(fs::path filepath) {
		if (!_sffile.openFromFile(filepath)) {
			Logger::error << "Failed to read length, skipping: " << filepath << ".";
			return;
		}
		uint64_t sample_cnt = _sffile.getSampleCount() - _sffile.getSampleCount() % _sffile.getChannelCount();

		FilenameRes file_type;
		if (auto res = filename2keyword(filepath.filename().wstring()); !res) {
			Logger::error << "Failed to handle on file: " << filepath << ".";
			return;
		}
		else {
			file_type = *res;
		}

		PAIRDATA& data = g_pairs[file_type.key];
		if (file_type.is_intro) {
			Logger::info << "Consider " << filepath << " as the 'intro' part of '" << file_type.key << "'.";
			if (data.has_intro) {
				Logger::warning << "Repeated 'intro' part of '" << file_type.key << "', ignoring.";
				return;
			}
			data.has_intro = true;
			data.offset = sample_cnt;
			data.intro_filepath = filepath;
		}
		else {
			Logger::info << "Consider " << filepath << " as the 'loop' part of '" << file_type.key << "'.";
			if (data.has_loop) {
				Logger::warning << "Repeated 'loop' part of '" << file_type.key << "', ignoring.";
				return;
			}
			data.has_loop = true;
			data.length = sample_cnt;
			data.loop_filepath = filepath;
		}
		return;
	}

	void process_pairs() {
		fs::create_directory(output_dir_path);
		for (const auto& i : g_pairs) {
			const std::wstring& key = i.first;
			const PAIRDATA& data = i.second;

			try {
				process_item(key, data);
			}
			catch (const internal_exception& e) {
				Logger::Exception(e);
				Logger::Exception(e.description());
				Logger::error << "Failed to process '" << key << "', skipping.";
			}
			catch (const std::exception& e) {
				Logger::Exception(e);
				Logger::error << "Failed to process '" << key << "', skipping.";
			}
			catch (...) {
				Logger::Exception("Unknown exception.");
				Logger::error << "Failed to process '" << key << "', skipping.";
			}
		}

		fs::remove("mylist.txt");
		fs::remove("fadeout.wav");
	}

	void process_item(const std::wstring& key, const PAIRDATA& data) {
		Logger::info << "Processing '" << key << "'.";

		if (!data.has_loop) {
			Logger::error << "No 'loop' part found of '" << key << "' skipping.";
			return;
		}

		std::ofstream mylist;
		mylist.open("mylist.txt");
		if (!mylist.is_open()) {
			Logger::error << "Failed to open 'mylist.txt' when handling '" << key << "' skipping.";
			return;
		}

		uint64_t offset = 0;
		if (data.has_intro) {
			mylist << "file '" << data.intro_filepath.string() << "'" << std::endl;
			offset = data.offset;
		}
		else {
			mylist << R"(file '.\empty.wav')" << std::endl;
			offset = offset_of_empty_intro;
		}
		mylist << "file '" << data.loop_filepath.string() << "'" << std::endl;
		mylist << R"(file '.\fadeout.wav')" << std::endl;
		mylist.flush();
		mylist.close();

		//ffmpeg -i i2.wav -to 2.0 -af "afade=t=out:st=0.8:d=1" output.wav
		CmdArgBuilderW cab;
		cab << L"-loglevel" << L"warning";
		cab << L"-y";
		cab << L"-i" << data.loop_filepath;
		cab << L"-to" << L"2.0";
		cab << L"-af" << L"afade=t=out:st=0.8:d=1";
		cab << L"fadeout.wav";

		int ret_code = WinProcRunAndWait(FFMPEG_PATH, cab);
		if (ret_code != 0) {
			Logger::error << "FFmpeg does not exit normally. Code (1, " << ret_code << ").";
			return;
		}

		// example
		//ffmpeg -i test.ogg -map 0 -y -codec copy -metadata "DESCRIPTION=xxxx" -metadata "TITLE=xxxname" -metadata "COPYRIGHT=HYPERGRYPH" -metadata "ORGANIZATION=ARKNIGHTS" testoutput.ogg 
		cab.clear();
		cab << L"-loglevel" << L"warning";
		cab << L"-y";
		cab << L"-f" << L"concat";
		cab << L"-safe" << L"0";
		cab << L"-i" << L"mylist.txt";
		cab << L"-af" << L"afade=t=in:st=0:d=0.001";
		cab << L"-metadata" << std::format(L"OHMSSPD=<{0}|{1}>", offset, data.length);
		if (set_title) {
			cab << L"-metadata" << std::format(L"TITLE={0}", key);
		}
		if (set_copyright) {
			cab << L"-metadata" << std::format(L"COPYRIGHT={0}", COPYRIGHT);
		}
		if (set_organization) {
			cab << L"-metadata" << std::format(L"ORGANIZATION={0}", ORGANIZATION);
		}

		if (output_flac_not_ogg) {
			cab << L"-c:a" << L"flac";
			if (output_flac_compression_level != -1) {
				cab << L"-compression_level" << output_flac_compression_level;
			}
			cab << L"output.flac";
		}
		else {
			cab << L"-c:a" << L"libvorbis";
			if (output_ogg_audio_quality != -99) {
				cab << L"-aq" << output_ogg_audio_quality;
			}
			cab << L"output.ogg";
		}

		ret_code = WinProcRunAndWait(FFMPEG_PATH, cab);
		if (ret_code != 0) {
			Logger::error << "FFmpeg does not exit normally. Code (2, " << ret_code << ").";
			return;
		}

		if (output_flac_not_ogg) {
			fs::rename(L"output.flac", output_dir_path / (key + L".flac"));
		}
		else {
			fs::rename(L"output.ogg", output_dir_path / (key + L".ogg"));
		}
	}

};
