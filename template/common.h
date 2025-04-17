#pragma once
#define DEBUGMODE 1
#define GAME 2

#define GAMETYPE DEBUGMODE

#if GAMETYPE == DEBUGMODE
constexpr int SCRWIDTH = 1280;
constexpr int SCRHEIGHT = 720;
#else
constexpr int SCRWIDTH = 480;
constexpr int SCRHEIGHT = 270;
#endif

constexpr int UPSCALE = 3;

constexpr int POINTLIGHTS = 4;
constexpr int WIDTHXHEIGHT = 921600;
constexpr float GRAVITY = -9.81;
constexpr double PI = 3.14159265358979323846264f;
constexpr double INVPI = 0.31830988618379067153777f;
constexpr double INV2PI	=	0.15915494309189533576888f;
constexpr double TWOPI	=	6.28318530717958647692528f;
constexpr double SQRT_PI_INV =	0.56418958355f;
constexpr double LARGE_FLOAT =	1e34f;
constexpr float EPSILON = 0.01f;