#ifndef _BITU_H_
#define _BITU_H_

#include "Aakar.h"
#include "Rang.h"
//#include "Vector.h"

namespace Gfx {

enum class BitmapFormat {
    Invalid,
    Indexed1,
    Indexed2,
    Indexed4,
    Indexed8,
    BGRx8888,
    BGRA8888,
    RGBA8888,
};

enum class StorageFormat {
    Indexed8,
    BGRx8888,
    BGRA8888,
    RGBA8888,
};


/*static*/ StorageFormat determine_storage_format(BitmapFormat format);
/*{
    switch (format) {
    case BitmapFormat::BGRx8888:
        return StorageFormat::BGRx8888;
    case BitmapFormat::BGRA8888:
        return StorageFormat::BGRA8888;
    case BitmapFormat::RGBA8888:
        return StorageFormat::RGBA8888;
    case BitmapFormat::Indexed1:
    case BitmapFormat::Indexed2:
    case BitmapFormat::Indexed4:
    case BitmapFormat::Indexed8:
        return StorageFormat::Indexed8;
    default:
       assert(false);// VERIFY_NOT_REACHED();
    }
}*/

//struct BackingStore;

struct BackingStore {
    void* data { nullptr };
    /*size_t*/int pitch { 0 };
    /*size_t*/int size_in_bytes { 0 };
};

enum RotationDirection {
    Left,
    Right
};

class Bitmap {//: public RefCounted<Bitmap> {
public:
    enum class ShouldCloseAnonymousFile {
        No,
        Yes,
    };

static Bitmap create(BitmapFormat, const IntSize&, int intrinsic_scale = 1);

static /*size_t*/ int palette_size(BitmapFormat format)
    {
        switch (format) {
        case BitmapFormat::Indexed1:
            return 2;
        case BitmapFormat::Indexed2:
            return 4;
        case BitmapFormat::Indexed4:
            return 16;
        case BitmapFormat::Indexed8:
            return 256;
        default:
            return 0;
        }
    }
    
        IntSize size() const { return m_size; }
    int scale() const { return m_scale; }
    IntSize physical_size() const { return size() * scale(); }
    int physical_height() const { return physical_size().height(); }

	    static constexpr /*size_t*/ int size_in_bytes(/*size_t*/int pitch, int physical_height) { return pitch * physical_height; }
    /*size_t*/int size_in_bytes() const { return size_in_bytes(m_pitch, physical_height()); }
    static /*size_t*/int minimum_pitch(/*size_t*/int physical_width, BitmapFormat);


private:
    enum class Purgeable {
        No,
        Yes
    };

	Bitmap(BitmapFormat, const IntSize&, int, Purgeable, /*const*/ BackingStore);
static BackingStore allocate_backing_store(BitmapFormat, const IntSize&, int, Purgeable);

//    void allocate_palette_from_format(BitmapFormat, const Vector<RGBA32>& source_palette);

	IntSize m_size;
    int m_scale;
    void* m_data { nullptr };
    RGBA32* m_palette { nullptr };
   /* size_t*/ int m_pitch { 0 };
    BitmapFormat m_format { BitmapFormat::Invalid };
    bool m_needs_munmap { false };
    bool m_purgeable { false };
    bool m_volatile { false };
    int m_anon_fd { -1 };

};

}

#endif
