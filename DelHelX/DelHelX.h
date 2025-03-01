// DelHelX.h : main header file for the DelHelX DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CDelHelApp
// See DelHelX.cpp for the implementation of this class
//

class CDelHelXApp : public CWinApp
{
public:
	CDelHelXApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
