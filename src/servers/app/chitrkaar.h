#ifndef _CHITRKAAR_H
#define _CHITRKAAR_H

#include "Aayat.h"
using namespace Gfx;
#include "Point.h"
#include "Orientation.h"
#include "Rang.h"
#include "CharacterBitmap.h"

/*template<typename T>
constexpr T min(const T& a, const T& b)
{
    return b < a ? b : a;
}

template<typename T>
constexpr T max(const T& a, const T& b)
{
    return a < b ? b : a;
}
*/




class Chitrkaar {
public:

Chitrkaar(unsigned int* addr, int width, int height);
  enum class LineStyle {
        Solid,
        Dotted,
        Dashed,
    };

	enum class DrawOp {
		Copy,
		Xor,
		Invert
	};
	
	DrawOp draw_op(){return DrawOp::Copy;}

unsigned int* scanline(int y);
void fill_rect_with_draw_op(const Int_Rect& a_rect, Color color);
void fill_physical_rect(const Int_Rect& physical_rect, int color);
void fill_rect_with_gradient(Orientation orientation, const Int_Rect& a_rect, Color gradient_start, Color gradient_end);
void draw_physical_pixel(const Int_Point& physical_position, int color, int thickness);
void fill_ellipse(const Int_Rect& a_rect, Color color);
void draw_bitmap(const Int_Point&, const CharacterBitmap&, Color = Color());
void draw_triangle(const Int_Point& a, const Int_Point& b, const Int_Point& c, Color color);

void set_physical_pixel_with_draw_op(unsigned int& pixel, const Color& color);

int pitch(){return scr_width;}


private:

	struct State {
		//const Font* font;
		Int_Point translation;
		int scale = 1;
		Int_Rect clip_rect;
		DrawOp draw_op;
	};

unsigned int *fb_addr;
int scr_width;
int scr_height;
};
#endif
