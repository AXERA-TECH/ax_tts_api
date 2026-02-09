/**************************************************************************************************
 *
 * Copyright (c) 2019-2026 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <codecvt>
#include <locale>
#include <cassert>

namespace utils {

std::vector<std::string> split_utf8(const std::string& utf8_text);

// https://github.com/bootphon/phonemizer/blob/master/phonemizer/utils.py#L35
std::vector<std::string> str2list(const std::string& text, char delimiter='\n');

void replace_inplace(std::string& str, const std::string& from, const std::string& to);

std::string strip(const std::string& s);

// 简单的 UTF-8 <-> UTF-16 转换
// 注意: std::codecvt_utf8_utf16 在 C++17 被标记为 deprecated，但它是目前最便携的标准方案。
void u8tou16(const char* src, size_t len, std::u16string& dst);

void u16tou8(const char16_t* src, size_t len, std::string& dst);

void SplitString(const std::string& str, char delimiter, std::vector<std::string>* result);

void SplitString(const char* str, size_t len, char delimiter, std::vector<std::string>* result);

std::string DigitToChinese(char c);

// 简单的数字转中文逻辑
// 支持整数和小数
// 例如: 123 -> 一百二十三, 3.14 -> 三点一四
// 增强: 支持 IP 地址或版本号 (1.2.3.4) -> 一点二点三点四
std::string NumberToChinese(const std::string& num_str);

bool is_chinese(const std::string& str);

std::string trim(const std::string& str);

std::string replace_all(std::string str, const std::string& from, const std::string& to);

std::string join(const std::vector<std::string>& vec, const std::string& delim);

}