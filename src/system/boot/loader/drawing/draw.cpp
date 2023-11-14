
/**
 * @file
 * @brief               Framebuffer console implementation.
 */
//#include <assert.h>

#include "draw.h"
//#include "font.h"

//#include "string.h"
//#include "utility.h"

//#include <assert.h>
//#include <loader.h>
//#include "memory.h"//for memory_alloc but i changed to malloc as of now...
//#include <video.h>

#include <boot/kernel_args.h>

#include <boot/stage2.h>
//#include "vid.h"
#include <stdlib.h>
#include <boot/platform/generic/video.h>
#include <boot/stdio.h>
#include <boot/platform.h>

#define TRACE_FB
#ifdef TRACE_FB
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

//video_mode_t *current_video_mode;

/** Framebuffer character information. */
typedef struct fb_char {
  char ch;                            /**< Character to display (0 == space). */
  uint8_t fg;                         /**< Foreground color. */
  uint8_t bg;                         /**< Background color. */
} fb_char_t;


/** Get the byte offset of a pixel.
 * @param x             X position of pixel.
 * @param y             Y position of pixel.
 * @return              Byte offset of the pixel. */
/*static inline size_t fb_offset(uint32_t x, uint32_t y) {
  return (y * current_video_mode->pitch) + (x * (current_video_mode->bpp >> 3));
}*/


/** Put a pixel on the framebuffer.
 * @param x             X position.
 * @param y             Y position.
 * @param rgb           RGB color to draw. */
  void fb_putpixel(uint16_t x, uint16_t y, uint32_t rgb) {
	size_t offset = (y * gKernelArgs.frame_buffer.width) + x;
	void *main = (void*)gKernelArgs.frame_buffer.physical_buffer.start;//fb->mapping + offset;
//  void *back = fb->backbuffer + offset;
//  uint32_t value = rgb888_to_fb(rgb);
//  uint32_t value = rgb | (0xff<<24) ;

	TRACE(("fb_putpixel  depth: %d\n", gKernelArgs.frame_buffer.depth));
	TRACE(("fb_putpixel  width: %d\n", gKernelArgs.frame_buffer.width));
	TRACE(("fb_putpixel  height: %d\n", gKernelArgs.frame_buffer.height));

  /*switch (current_video_mode->bpp >> 3) {
  case 2:
    *(uint16_t *)main = (uint16_t)value;
    *(uint16_t *)back = (uint16_t)value;
    break;
  case 3:
    ((uint16_t *)main)[0] = value & 0xffff;
    ((uint8_t *)main)[2] = (value >> 16) & 0xff;
    ((uint16_t *)back)[0] = value & 0xffff;
    ((uint8_t *)back)[2] = (value >> 16) & 0xff;
    break;
  case 4:
    *(uint32_t *)main = value;
    *(uint32_t *)back = value;
    break;
  }*/
	uint32* start=(uint32*)main;
	// TODO what about other depths...
	if (gKernelArgs.frame_buffer.depth == 32){
		start = start + offset;
		*start = rgb;
	}
   
 /*int i=0;
 while(i<50000)
 {
 *start=0xffff0000;
 start++;
 i++;
 }*/

}


