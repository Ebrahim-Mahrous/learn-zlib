#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef _DEBUG
#define DEBUG(x) if (!(x)) return -1
#define CHECK(x, n) if ((n = (x)) < 0) return n
#else
#define DEBUG(x)
#define CHECK(x, n) x
//#define CHECK(x, n) x
#endif

#endif