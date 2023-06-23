#include "Bitu.h"
using namespace Gfx;
//#include "Checked.h"
#include <sys/mman.h>

//struct BackingStore {
//    void* data { nullptr };
//    /*size_t*/int pitch { 0 };
//    /*size_t*/int size_in_bytes { 0 };
//};

StorageFormat Gfx::determine_storage_format(BitmapFormat format)
{
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
}

static bool size_would_overflow(BitmapFormat format, const IntSize& size, int scale_factor)
{
    if (size.width() < 0 || size.height() < 0)
        return true;
    // This check is a bit arbitrary, but should protect us from most shenanigans:
    if (size.width() >= 32768 || size.height() >= 32768 || scale_factor < 1 || scale_factor > 4)
        return true;
    // In contrast, this check is absolutely necessary:
//   /* size_t*/int pitch = Bitmap::minimum_pitch(size.width() * scale_factor, format);
    return false;//Checked</*size_t*/int>::multiplication_would_overflow(pitch, size.height() * scale_factor);
}


/*size_t*/int Bitmap::minimum_pitch(/*size_t*/int physical_width, BitmapFormat format)
{
    /*size_t*/int element_size;
    switch (determine_storage_format(format)) {
    case StorageFormat::Indexed8:
        element_size = 1;
        break;
    case StorageFormat::BGRx8888:
    case StorageFormat::BGRA8888:
    case StorageFormat::RGBA8888:
        element_size = 4;
        break;
    default:
        assert(false);//VERIFY_NOT_REACHED();
    }

    return physical_width * element_size;
}



Bitmap Bitmap::create(BitmapFormat format, const IntSize& size, int scale_factor)
{
//    auto backing_store = Bitmap::allocate_backing_store(format, size, scale_factor, Purgeable::No);
BackingStore backing_store = allocate_backing_store(format, size, scale_factor, Purgeable::No);
 //   if (!backing_store.has_value())
 //       return nullptr;
 Bitmap bitu2=Bitmap(format, size, scale_factor, Purgeable::No, backing_store);
  
  //Bitmap* bitu=&bitu2;
    return bitu2;
    //bitu;///*adopt(*new*/ Bitmap(format, size, scale_factor, Purgeable::No, backing_store);//);//backing_store.value()));
}



 Bitmap::Bitmap(BitmapFormat format, const IntSize& size, int scale_factor, Purgeable purgeable,/* const*/ BackingStore backing_store)
    : m_size(size)
    , m_scale(scale_factor)
    , m_data(backing_store.data)
    , m_pitch(backing_store.pitch)
    , m_format(format)
    , m_purgeable(purgeable == Purgeable::Yes)
{
    assert(!m_size.is_empty());
    assert(!size_would_overflow(format, size, scale_factor));
    assert(m_data);
    assert(backing_store.size_in_bytes == size_in_bytes());
   // int size = palette_size(format);
    //allocate_palette_from_format(format, RGBA[] //{});
    m_needs_munmap = true;
}




BackingStore Bitmap::allocate_backing_store(BitmapFormat format, const IntSize& size, int scale_factor, Purgeable purgeable)
{
    if (size_would_overflow(format, size, scale_factor))
        return {};

    const auto pitch = minimum_pitch(size.width() * scale_factor, format);
    const auto data_size_in_bytes = size_in_bytes(pitch, size.height() * scale_factor);

    int map_flags = MAP_ANONYMOUS | MAP_PRIVATE;
    if (purgeable == Purgeable::Yes)
        map_flags |= MAP_NORESERVE;
/*#ifdef __serenity__
    void* data = mmap_with_name(nullptr, data_size_in_bytes, PROT_READ | PROT_WRITE, map_flags, 0, 0, String::format("GraphicsBitmap [%dx%d]", size.width(), size.height()).characters());
#else
*/
    void* data = mmap(nullptr, data_size_in_bytes, PROT_READ | PROT_WRITE, map_flags, 0, 0);
//#endif
    if (data == MAP_FAILED) {
     //   perror("mmap");
        return {};
    }
    
    //BackingStore bs1;
    //bs1.data=data;
    //bs1.pitch=pitch;
    //bs1.size_in_bytes=data_size_in_bytes;
    
    
    //={data,pitch,data_size_in_bytes};
  //  BackingStore *bs=&bs1;
    return {  data, pitch, data_size_in_bytes };
}


//void Bitmap::allocate_palette_from_format(BitmapFormat format, RGBA32[] src//const Vector<RGBA32>& source_palette)
//{
//    /*size_t*/ int size = palette_size(format);
//    if (size == 0)
//        return;
//    m_palette = new RGBA32[size];
//    if (!source_palette.is_empty()) {
 //       assert(source_palette.size() == size);
//        memcpy(m_palette, source_palette.data(), size * sizeof(RGBA32));
  //  }
//}
