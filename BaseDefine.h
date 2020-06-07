#ifndef MANIPULATE_BITMAP_BASE_DEFINE_H_
#define MANIPULATE_BITMAP_BASE_DEFINE_H_

#ifdef _DEBUG
#define MANIPULATE_BITMAP_DEBUG
#endif // _DEBUG

#ifdef MANIPULATE_BITMAP_DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <cassert>
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#endif // MANIPULATE_BITMAP_DEBUG

#endif // MANIPULATE_BITMAP_BASE_DEFINE_H_