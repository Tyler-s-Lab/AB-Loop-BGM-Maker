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

import PairDataModule;
import ArknightsModule;

namespace fs = std::filesystem;

export class Work {
	const size_t Max_Recurse_Depth = 8;
	const std::string FFMPEG_PATH = "ffmpeg";

	sf::InputSoundFile _sffile;
	uint64_t offset_of_empty_intro = 0;

	fs::path output_dir_path;

	bool set_title = false;
	bool set_copyright = false;
	bool set_organization = false;
	bool output_flac_not_ogg = false;
	int output_flac_compression_level = -1;
	int output_ogg_audio_quality = -99;

	std::map<std::string, PAIRDATA> g_pairs; // 音乐名对数据结构的map

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
			Logger::Warning(std::format(L"Config file missing: '{0}'.", ini_path.wstring()));
			return;
		}

		std::shared_ptr<ini::IniConfig> g_iniConfig;
		if (auto res = ini::read_ini_file(ini_path.string()); !res) {
			Logger::Warning(std::format(L"Config file corrupted: '{0}'.", ini_path.wstring()));
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
							Logger::Warning(std::format(L"Invalid flac_compression_level (0 - 8) in ini file, ignored: '{0}'.", *pp));
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
							Logger::Warning(std::format(L"Invalid ogg_quality (0 - 10) in ini file, ignored: '{0}'.", *pp));
						}
						else {
							output_ogg_audio_quality = static_cast<int>(*pp);
						}
					}
				}
				else {
					Logger::Warning(std::format("Invalid format in ini file, defaulting to ogg: '{0}'.", *p));
				}
			}
		}
	}

	void enum_one_item(fs::path input) {
		static size_t depth = 0;
		if (depth >= Max_Recurse_Depth) {
			Logger::Warning(std::format(L"Reached Max_Recurse_Depth ({0}), skipping: '{1}'.", Max_Recurse_Depth, input.wstring()));
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
			Logger::Warning(std::format(L"Unknown item, skipping: '{0}'.", input.wstring()));
		}

		depth--;
		return;
	}

	void enum_one_file(fs::path filepath) {
		if (!_sffile.openFromFile(filepath)) {
			Logger::Warning(std::format(L"Failed to read length, skipping: '{0}'.", filepath.wstring()));
			return;
		}
		uint64_t sample_cnt = _sffile.getSampleCount() - _sffile.getSampleCount() % _sffile.getChannelCount();

		FilenameRes file_type;
		if (auto res = filename2keyword(filepath.filename().string()); !res) {
			Logger::Warning(std::format(L"Failed to handle on file: '{0}'.", filepath.wstring()));
			return;
		}
		else {
			file_type = *res;
		}

		PAIRDATA& data = g_pairs[file_type.key];
		if (file_type.is_intro) {
			Logger::Info(std::format("Consider '{0}' as the 'intro' part of '{1}'.", filepath.string(), file_type.key));
			if (data.has_intro) {
				Logger::Warning(std::format("Repeated 'intro' part of '{0}'.", file_type.key));
				return;
			}
			data.has_intro = true;
			data.offset = sample_cnt;
			data.intro_filepath = filepath;
		}
		else {
			Logger::Info(std::format("Consider '{0}' as the 'loop' part of '{1}'.", filepath.string(), file_type.key));
			if (data.has_loop) {
				Logger::Warning(std::format("Repeated 'loop' part of '{0}'.", file_type.key));
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
		std::ofstream mylist;
		for (const std::pair<std::string, PAIRDATA>& i : g_pairs) {
			const std::string& key = i.first;
			const PAIRDATA& data = i.second;

			Logger::Info(std::format("Processing '{0}'.", key));

			if (!data.has_loop) {
				Logger::Warning(std::format("No 'loop' part found of '{0}', skipping.", key));
				continue;
			}

			mylist.open("mylist.txt");
			if (!mylist.is_open()) {
				Logger::Error(std::format("Failed to open 'mylist.txt' when handling '{0}', skipping.", key));
				continue;
			}

			uint64_t offset = 0;
			if (data.has_intro) {
				mylist << "file \'" << data.intro_filepath.string() << '\'' << std::endl;
				offset = data.offset;
			}
			else {
				mylist << "file .\\\\empty.wav" << std::endl;
				offset = offset_of_empty_intro;
			}
			mylist << "file \'" << data.loop_filepath.string() << '\'' << std::endl;
			mylist << "file \'.\\fadeout.wav\'" << std::endl;

			//ffmpeg -i i2.wav -to 2.0 -af "afade=t=out:st=0.8:d=1" output.wav
			system(
				(FFMPEG_PATH + " -loglevel warning -y -i \"" +
					data.loop_filepath.string() +
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

			if (output_flac_not_ogg) {
				tmpcmd.append(" -c:a flac");
				if (output_flac_compression_level != -1) {
					tmpcmd.append(" -compression_level " + std::to_string(output_flac_compression_level));
				}
				tmpcmd.append(" output.flac");

				system(tmpcmd.c_str());
				fs::rename("output.flac", output_dir_path / (key + ".flac"));
			}
			else {
				tmpcmd.append(" -c:a libvorbis");
				if (output_ogg_audio_quality != -99) {
					tmpcmd.append(" -aq " + std::to_string(output_ogg_audio_quality));
				}
				tmpcmd.append(" output.ogg");

				system(tmpcmd.c_str());
				fs::rename("output.ogg", output_dir_path / (key + ".ogg"));
			}

			mylist.close();
		}

		fs::remove("mylist.txt");
		fs::remove("fadeout.wav");
	}

};
