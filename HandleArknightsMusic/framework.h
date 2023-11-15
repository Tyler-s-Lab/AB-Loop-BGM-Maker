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
#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


#define FFMPEG_PATH (".\\core\\ffmpeg.exe")

// Using library SFML to get audio duration.
// Duration data will be output into ‘comments.txt’ in output directory.
// It is optinal, not necessary.
// 
// 使用SFML库获取音频长度。
// 音频长度信息会输出至输出目录下的‘comments.txt’里。
// 这是一个可选项，非必须。
#define HAVE_SFML

// Also write duration data into metadata (tag) of output .ogg files.
// These files with metadata will be in ‘ogg_metadata’ in output directory.
// It is optinal, not necessary.
// 
// 同时将音频长度信息写入输出的.ogg文件的元数据。
// 写入元数据的文件会被放入输出目录下的‘ogg_metadata’目录。
// 这是一个可选项，非必须。
#define WRITE_METADATA

