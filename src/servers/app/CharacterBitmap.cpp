
#include "CharacterBitmap.h"

namespace Gfx {

CharacterBitmap::CharacterBitmap(const char* ascii_data, unsigned width, unsigned height)
    : m_bits(ascii_data)
    , m_size(width, height)
{
}

CharacterBitmap::~CharacterBitmap()
{
}

//NonnullRefPtr<CharacterBitmap> 
CharacterBitmap CharacterBitmap::create_from_ascii(const char* asciiData, unsigned width, unsigned height)
{
CharacterBitmap cb=CharacterBitmap(asciiData, width, height);
    return cb;//new CharacterBitmap(asciiData, width, height);
    //adopt(*new CharacterBitmap(asciiData, width, height));
}

}
