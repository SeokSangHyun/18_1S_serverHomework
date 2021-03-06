#pragma once

#include "resource.h"
#include "stdafx.h"


//WNDCLASSEXW 초기화
ATOM                MyRegisterClass(HINSTANCE hInstance);

//윈도우를 초기화함.
BOOL                InitInstance(HINSTANCE, int);

//Main Window Procedure
//메시지 처리를함
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);


INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
//


void BuildSettting(RECT **board)
{
	float div_x = BOARD_WIDTH / BOARD_COUNT;
	float div_y =  BOARD_HEIGHT / BOARD_COUNT;

	for (int i = 0; i < BOARD_COUNT; ++i){
		for (int j = 0; j < BOARD_COUNT; ++j){
			board[i][j].left = j * div_x;
			board[i][j].right = j * div_x + div_x;

			board[i][j].top = i * div_y;
			board[i][j].bottom = i * div_y + div_y;
		}
	}
	//
}



void Render();
/*
*/
void PrintBoard(HDC *dc, RECT **board)
{
	HBRUSH whiteBrush, blackBrush;
	HBRUSH myBrush;
	HPEN myPen, oldPen;
	//가로
	blackBrush = CreateSolidBrush(RGB(180, 180, 180));
	whiteBrush = CreateSolidBrush(RGB(200, 200, 200));
	oldPen = (HPEN)CreatePen(PS_SOLID, 2, RGB(150, 150, 150));

	for (int i = 0; i < BOARD_COUNT; ++i)
	{
		for (int j = 0; j < BOARD_COUNT; ++j)
		{
			if ((i + j) % 2 ? true : false) myBrush = (HBRUSH)SelectObject(*dc, whiteBrush);
			else							myBrush = (HBRUSH)SelectObject(*dc, blackBrush);
			Rectangle(*dc, board[i][j].left, board[i][j].top, board[i][j].right, board[i][j].bottom);
			SelectObject(*dc, myBrush);
		}
	}

	SelectObject(*dc, GetStockObject(NULL_BRUSH));
	for (int i = 0; i < BOARD_COUNT; ++i)
	{
		for (int j = 0; j < BOARD_COUNT; ++j)
		{
			myPen = (HPEN)SelectObject(*dc, oldPen);//지금 사용할 팬을 정함
			Rectangle(*dc, board[i][j].left, board[i][j].top, board[i][j].right, board[i][j].bottom);
			SelectObject(*dc, myPen);
		}
	}


	DeleteObject(oldPen);
	DeleteObject(whiteBrush);
	DeleteObject(blackBrush);
}