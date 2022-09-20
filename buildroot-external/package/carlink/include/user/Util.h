#ifndef __UTIL__
#define __UTIL__
#include <string>
#include <vector>
#include <regex>
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <sstream>

std::string& Strim(std::string &s, const std::string & del);
std::vector<std::string> Split(const std::string & input, const std::string& regex);
int string2Digtal(std::string str);


#endif //__UTIL__

