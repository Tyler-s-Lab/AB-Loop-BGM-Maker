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

#include "framework.h"

/**
 * @brief Output directory
 * @brief 输出目录
*/
extern std::string g_outputpath;

/**
 * @brief Process function
 * @brief 处理函数
 * @return true if success
 * @return 函数是否成功
*/
bool process();
