#pragma once
#include <iostream>
#include "engine/engine_defines.hpp"
using namespace std;
using namespace Engine;

class GameTest
{
public:
	static void TestDataFactory();
	static void TestLog();
	static void TestBuffer();
	static void TestProtoBuffer();
	static void TestDispatcher();
};
