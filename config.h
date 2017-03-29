/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#pragma once

//#include <fstream>
//#include <string>
//
//
//class config
//{
//public:
//	config()
//	{
//		auto fin = std::ifstream("config.txt");
//
//		std::string line;
//		while (std::getline(fin, line))
//		{
//			if (strip_prefix(line, "recomp_T="))
//				recomp_T = std::stoi(line);
//			else if (strip_prefix(line, "use_swap="))
//				use_swap = (line != "0");
//			else if (strip_prefix(line, "use_reverse="))
//				use_reverse = (line != "0");
//			else if (strip_prefix(line, "use_insert="))
//				use_insert = (line != "0");
//			else if (strip_prefix(line, "max_rev="))
//				max_rev = std::stoi(line.c_str());
//			else if (strip_prefix(line, "max_ins="))
//				max_ins = std::stoi(line.c_str());
//			else if (strip_prefix(line, "init_T="))
//				init_T = std::stod(line.c_str());
//			else if (strip_prefix(line, "last_T="))
//				last_T = std::stod(line.c_str());
//			else if (strip_prefix(line, "K="))
//				K = std::stod(line.c_str());
//			else if (strip_prefix(line, "iterations="))
//				iterations = std::stoi(line.c_str());
//			else if (strip_prefix(line, "seed="))
//				seed = std::stoi(line.c_str());
//			else if (strip_prefix(line, "debug="))
//				debug = (line != "0");
//		}
//
//		if (debug) print();
//	}
//
//	template <typename T>
//	static bool begins_with(const std::basic_string<T>& str, const T *prefix, size_t length)
//	{
//		return str.compare(0, length, prefix, length) == 0;
//	}
//
//	template <typename T>
//	static bool strip_prefix(std::basic_string<T>& str, const T *prefix)
//	{
//		auto length = std::char_traits<T>::length(prefix);
//		if (begins_with(str, prefix, length))
//		{
//			str.erase(0, length);
//			return true;
//		}
//		return false;
//	}
//
//	void print()
//	{
//		std::cout << "recomp_T:  " << recomp_T << std::endl;
////		std::cout << "use_swap:  " << std::boolalpha << use_swap << std::endl;
////		std::cout << "use_rever: " << std::boolalpha << use_reverse << std::endl;
////		std::cout << "use_ins:   " << std::boolalpha << use_insert << std::endl;
//		std::cout << "max_rever: " << max_rev << std::endl;
//		std::cout << "max_ins:   " << max_ins << std::endl;
//		std::cout << "init_T:    " << init_T << std::endl;
//		std::cout << "last_T:    " << last_T << std::endl;
//		std::cout << "K:         " << K << std::endl;
//		std::cout << "iterat.:   " << iterations << std::endl;
////		std::cout << "seed:      " << seed << std::endl;
//	}
//
//	bool debug = false;
//	int recomp_T = true;
//
//	bool use_swap = true;
//	bool use_reverse = true;
//	bool use_insert = true;
//
//	int max_ins = 1000;
//	int max_rev = 1000;
//
//	double init_T = 0;
//	double last_T = 0.002;
//	double K = 0;
//
//	int iterations;
//
//	int seed = 0;
//};
//
