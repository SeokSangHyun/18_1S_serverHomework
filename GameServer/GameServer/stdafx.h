#pragma once

#define WIN32_LEAN_AND_MEAN  
#define INITGUID

#include <winsock2.h>
#pragma comment(lib, "ws2_32")

#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <thread>

#include <Mmsystem.h>
#include <commctrl.h>
#include <stdlib.h>
#include <string>
#include <unordered_set>
#include <vector>
#include <array>

#include "targetver.h"
#include "..\..\protocol.h"
#pragma comment(lib, "winmm.lib")

#define BUFSIZE    512
#define VIEW_RADIUS 30


enum DataType {notLink, Link};
enum CharDir {stay = 10, left, right, up, down };




////////////////////
void error_display(const char *msg, int err_no);


