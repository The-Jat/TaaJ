#ifndef _SIMD_H
#define _SIMD_H

//#include <AK/Types.h>

namespace SIMD {

using i8x2 = char __attribute__((vector_size(2)));
using i8x4 = char __attribute__((vector_size(4)));
using i8x8 = char __attribute__((vector_size(8)));
using i8x16 = char __attribute__((vector_size(16)));
using i8x32 = char __attribute__((vector_size(32)));

using i16x2 = short __attribute__((vector_size(4)));
using i16x4 = short __attribute__((vector_size(8)));
using i16x8 = short __attribute__((vector_size(16)));
using i16x16 = short __attribute__((vector_size(32)));

using i32x2 = int __attribute__((vector_size(8)));
using i32x4 = int __attribute__((vector_size(16)));
using i32x8 = int __attribute__((vector_size(32)));

using i64x2 = long long int __attribute__((vector_size(16)));//i64
using i64x4 = long long int __attribute__((vector_size(32)));

using u8x2 = unsigned char __attribute__((vector_size(2)));
using u8x4 = unsigned char __attribute__((vector_size(4)));
using u8x8 = unsigned char __attribute__((vector_size(8)));
using u8x16 = unsigned char __attribute__((vector_size(16)));
using u8x32 = unsigned char __attribute__((vector_size(32)));

using u16x2 = unsigned short __attribute__((vector_size(4)));
using u16x4 = unsigned short __attribute__((vector_size(8)));
using u16x8 = unsigned short __attribute__((vector_size(16)));
using u16x16 = unsigned short __attribute__((vector_size(32)));

using u32x2 = unsigned int __attribute__((vector_size(8)));
using u32x4 = unsigned int __attribute__((vector_size(16)));
using u32x8 = unsigned int __attribute__((vector_size(32)));

using u64x2 = unsigned long long int __attribute__((vector_size(16)));
using u64x4 = unsigned long long int __attribute__((vector_size(32)));

using f32x2 = float __attribute__((vector_size(8)));
using f32x4 = float __attribute__((vector_size(16)));
using f32x8 = float __attribute__((vector_size(32)));

using f64x2 = double __attribute__((vector_size(16)));
using f64x4 = double __attribute__((vector_size(32)));

}

#endif
