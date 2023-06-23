#include "chitrkaar.h"
#include "Gamma.h"
#include "StdLibExtras.h"
#include "Bitu.h"

Chitrkaar::Chitrkaar(unsigned int * addr, int width, int height)
{
	fb_addr = addr;
	scr_width = width;
	scr_height = height;

	IntSize m_desktop_resolution{width,height};
	Int_Rect screen_rect = { { 0, 0 }, m_desktop_resolution };
//Bitmap m_monitor_bitmap;
//BitmapFormat form=RGBA8888;
auto screen_bitmap = Gfx::Bitmap::create(/*m_monitor_bitmap->format()*/BitmapFormat::RGBA8888, m_desktop_resolution);
}


unsigned int* Chitrkaar::scanline(int y)
{
	assert(y>=0 && y< scr_width);
	return (fb_addr + y * scr_width);
}


void Chitrkaar::fill_rect_with_draw_op(const Int_Rect& a_rect, Color color)
{
    //VERIFY(scale() == 1); // FIXME: Add scaling support.

    //auto rect = a_rect.translated(translation()).intersected(clip_rect());
    //if (rect.is_empty())
     //   return;

    RGBA32* dst = scanline(a_rect.top()) + a_rect.left();
    const int dst_skip = pitch();

    for (int i = a_rect.height() - 1; i >= 0; --i) {
        for (int j = 0; j < a_rect.width(); ++j)
            //dst[j]=color;//
            set_physical_pixel_with_draw_op(dst[j], color);
        dst += dst_skip;
    }
}


void Chitrkaar::fill_physical_rect(const Int_Rect& physical_rect, int color)
{
//top left points
int x=max(physical_rect.top(),0);
int y=max(physical_rect.left(),0);
//top right points
//int x2=min(physical_rect.top()+width(),scr.width);
//int y2=

int offset=y * scr_width + x;
unsigned int * temp_fb = fb_addr;
temp_fb=temp_fb + offset;
    // Callers must do clipping.
//    RGBA32* dst = m_target->scanline(physical_rect.top()) + physical_rect.left();
//    const size_t dst_skip = m_target->pitch() / sizeof(RGBA32);

    for (int i = physical_rect.height() - 1; i >= 0; --i) {
        for (int j = 0; j < physical_rect.width(); ++j){
            *temp_fb = color;
            temp_fb++;
            
            //dst[j] = Color::from_rgba(dst[j]).blend(color).value();
        //dst += dst_skip;
        }
        temp_fb=temp_fb+scr_width-physical_rect.width();
    }
}



void Chitrkaar::fill_rect_with_gradient(Orientation orientation, const Int_Rect& a_rect, Color gradient_start, Color gradient_end)
{
//#ifdef NO_FPU
//    return fill_rect(a_rect, gradient_start);
//#endif

unsigned int * temp_fb = fb_addr;
int offset = a_rect.top() * scr_width + a_rect.left();
temp_fb = temp_fb + offset;
 //   auto rect = a_rect.translated(translation());
  //  auto clipped_rect = IntRect::intersection(rect, clip_rect());
  //  if (clipped_rect.is_empty())
   //     return;

    int offset1 = a_rect.primary_offset_for_orientation(orientation) - a_rect.primary_offset_for_orientation(orientation);

   // RGBA32* dst = m_target->scanline(clipped_rect.top()) + clipped_rect.left();
//    const size_t dst_skip = m_target->pitch() / sizeof(RGBA32);
int dst_skip = scr_width - a_rect.width();
    float increment = (1.0 / ((a_rect.primary_size_for_orientation(orientation))));

    if (orientation == Orientation::Horizontal) {
        for (int i = a_rect.height() - 1; i >= 0; --i) {
            float c = offset1 * increment;
            for (int j = 0; j < a_rect.width(); ++j) {
                *temp_fb = gamma_accurate_blend(gradient_start, gradient_end, c).value();
                c += increment;
                temp_fb++;
            }
            temp_fb += dst_skip;
        }
    } else {
        float c = offset1 * increment;
        for (int i = a_rect.height() - 1; i >= 0; --i) {
            auto color = gamma_accurate_blend(gradient_start, gradient_end, c);
            for (int j = 0; j < a_rect.width(); ++j) {
                *temp_fb = color.value();
                temp_fb++;
            }
            c += increment;
            temp_fb += dst_skip;
        }
    }
}


inline void Chitrkaar::set_physical_pixel_with_draw_op(unsigned int& pixel, const Color& color)
{
    // This always sets a single physical pixel, independent of scale().
    // This should only be called by routines that already handle scale.

    switch (draw_op()) {
    case DrawOp::Copy:
        pixel = color.value();
        break;
    case DrawOp::Xor:
        pixel = color.xored(Color::from_rgba(pixel)).value();
        break;
    case DrawOp::Invert:
        pixel = Color::from_rgba(pixel).inverted().value();
        break;
    }
}


void Chitrkaar::draw_physical_pixel(const Int_Point& physical_position, int color, int thickness)
{
	if(thickness == 1)
	{
		int offset = physical_position.y() * scr_width + physical_position.x();
		fb_addr = fb_addr + offset;
		*fb_addr = color;
	}

	//Int_Rect rect { physical_position, { thickness, thickness } };
	//rect.intersect(clip_rect() * scale());
}


void Chitrkaar::fill_ellipse(const Int_Rect& a_rect, Color color)
{
//    VERIFY(scale() == 1); // FIXME: Add scaling support.

 //   auto rect = a_rect.translated(translation()).intersected(clip_rect());
 //   if (rect.is_empty())
 //       return;

//    VERIFY(m_target->rect().contains(rect));

//unsigned int* temp_fb = fb_addr;
//int offset = scr_width * a_rect.top() + a_rect.left();
//temp_fb += offset;

    RGBA32* dst = scanline(a_rect.top()) + a_rect.left() + a_rect.width() / 2;
    const int dst_skip = pitch();

    for (int i = 0; i < a_rect.height(); i++) {
        double y = a_rect.height() * 0.5 - i;
        double x = a_rect.width() * sqrt(0.25 - y * y / a_rect.height() / a_rect.height());
        jaat_std::fast_u32_fill(dst - (int)x, color.value(), 2 * (int)x);
        dst += dst_skip;
       // temp_fb += scr_width-a_rect.width();
    }
}


void Chitrkaar::draw_bitmap(const Int_Point& p, const CharacterBitmap& bitmap, Color color)
{
  //  VERIFY(scale() == 1); // FIXME: Add scaling support.

    auto rect = Int_Rect(p, bitmap.size());//.translated(translation());
    Int_Rect clip_rect{0,0,1024,768};
    auto clipped_rect = rect.intersected(clip_rect);//clip_rect());
    //if (clipped_rect.is_empty())
     //   return;
    const int first_row = clipped_rect.top() - rect.top();
    const int last_row = clipped_rect.bottom() - rect.top();
    const int first_column = clipped_rect.left() - rect.left();
    const int last_column = clipped_rect.right() - rect.left();
    RGBA32* dst = scanline(clipped_rect.y()) + clipped_rect.x();
    const /*size_t*/int dst_skip = pitch();// / sizeof(RGBA32);
    const char* bitmap_row = &bitmap.bits()[first_row * bitmap.width() + first_column];
    const /*size_t*/int bitmap_skip = bitmap.width();

    for (int row = first_row; row <= last_row; ++row) {
        for (int j = 0; j <= (last_column - first_column); ++j) {
            char fc = bitmap_row[j];
            if (fc == '#')
                dst[j] = color.value();
        }
        bitmap_row += bitmap_skip;
        dst += dst_skip;
    }
}


void Chitrkaar::draw_triangle(const Int_Point& a, const Int_Point& b, const Int_Point& c, Color color)
{
    RGBA32 rgba = color.value();

    Int_Point p0(a);
    Int_Point p1(b);
    Int_Point p2(c);

    if (p0.y() > p1.y())
        jaat_std::swap(p0, p1);
    if (p0.y() > p2.y())
        jaat_std::swap(p0, p2);
    if (p1.y() > p2.y())
        jaat_std::swap(p1, p2);

    //auto clip = //clip_rect();
	Int_Rect clip={0,0,1024,768};
    if (p0.y() >= clip.bottom())
        return;
    if (p2.y() < clip.top())
        return;

    float dx01 = (float)(p1.x() - p0.x()) / (p1.y() - p0.y());
    float dx02 = (float)(p2.x() - p0.x()) / (p2.y() - p0.y());
    float dx12 = (float)(p2.x() - p1.x()) / (p2.y() - p1.y());

    float x01 = p0.x();
    float x02 = p0.x();

    int top = p0.y();
    if (top < clip.top()) {
        x01 += dx01 * (clip.top() - top);
        x02 += dx02 * (clip.top() - top);
        top = clip.top();
    }

unsigned int* fb_temp = fb_addr;
int offset = scr_width * top ;
fb_temp += offset;
    for (int y = top; y < p1.y() && y < clip.bottom(); ++y) {
        int start = x01 > x02 ? max((int)x02, clip.left()) : max((int)x01, clip.left());
        int end = x01 > x02 ? min((int)x01, clip.right()) : min((int)x02, clip.right());
        //auto* scanline = m_target->scanline(y);
        
        for (int x = start; x < end; x++) {
            //scanline[x] = rgba;
		fb_temp[x]=rgba;
        }
        x01 += dx01;
        x02 += dx02;
    }

    x02 = p0.x() + dx02 * (p1.y() - p0.y());
    float x12 = p1.x();

    top = p1.y();
    if (top < clip.top()) {
        x02 += dx02 * (clip.top() - top);
        x12 += dx12 * (clip.top() - top);
        top = clip.top();
    }

fb_temp = fb_addr;
offset = scr_width * top;
fb_temp += offset;
    for (int y = top; y < p2.y() && y < clip.bottom(); ++y) {
        int start = x12 > x02 ? max((int)x02, clip.left()) : max((int)x12, clip.left());
        int end = x12 > x02 ? min((int)x12, clip.right()) : min((int)x02, clip.right());
//        auto* scanline = m_target->scanline(y);
        for (int x = start; x < end; x++) {
            //scanline[x] = rgba;
		fb_temp[x] = rgba;
        }
        x02 += dx02;
        x12 += dx12;
    }
}
