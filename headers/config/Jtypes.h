#ifndef _J_CONFIG_TYPES_H
#define _J_CONFIG_TYPES_H


#include <config/JConfig.h>


/* fixed-width types -- the __j_std_[u]int* types correspond to the POSIX
   [u]int*_t types, the _j_[u]int* types to the BeOS [u]int* types. If
   __JS_BEOS_COMPATIBLE_TYPES is not defined both sets are identical. Once
   we drop compatibility for good, we can consolidate the types.
*/
typedef signed char			__j_std_int8;
typedef unsigned char		__j_std_uint8;
typedef signed short		__j_std_int16;
typedef unsigned short		__j_std_uint16;
typedef signed int			__j_std_int32;
typedef unsigned int		__j_std_uint32;
#ifdef __JS_ARCH_64_BIT
typedef signed long			__j_std_int64;
typedef unsigned long		__j_std_uint64;
#else
typedef signed long long	__j_std_int64;
typedef unsigned long long	__j_std_uint64;
#endif

typedef __j_std_int8	__j_int8;
typedef __j_std_uint8	__j_uint8;
typedef __j_std_int16	__j_int16;
typedef __j_std_uint16	__j_uint16;
#ifdef __JS_BEOS_COMPATIBLE_TYPES
typedef signed long int		__j_int32;
typedef unsigned long int	__j_uint32;
#else
typedef __j_std_int32	__j_int32;
typedef __j_std_uint32	__j_uint32;
#endif
typedef __j_std_int64	__j_int64;
typedef __j_std_uint64	__j_uint64;

/* address types */
typedef signed long int		__j_saddr_t;
typedef	unsigned long int	__j_addr_t;

#ifdef __JS_ARCH_PHYSICAL_64_BIT
	typedef __j_int64	__j_phys_saddr_t;
	typedef __j_uint64	__j_phys_addr_t;
#else
	typedef	__j_int32	__j_phys_saddr_t;
	typedef	__j_uint32	__j_phys_addr_t;
#endif

/* address type limits */
#ifdef __JS_ARCH_64_BIT
#	define __JS_SADDR_MAX		(9223372036854775807LL)
#	define __JS_ADDR_MAX			(18446744073709551615ULL)
#else
#	define __JS_SADDR_MAX		(2147483647)
#	define __JS_ADDR_MAX			(4294967295U)
#endif
#define __JS_SADDR_MIN			(-__JS_SADDR_MAX-1)

#ifdef __JS_ARCH_PHYSICAL_64_BIT
#	define __JS_PHYS_SADDR_MAX	(9223372036854775807LL)
#	define __JS_PHYS_ADDR_MAX	(18446744073709551615ULL)
#else
#	define __JS_PHYS_SADDR_MAX	(2147483647)
#	define __JS_PHYS_ADDR_MAX	(4294967295U)
#endif
#define __JS_PHYS_SADDR_MIN		(-__JS_SADDR_MAX-1)


/* printf()/scanf() format prefixes */
#define	__JS_STD_PRI_PREFIX_32	""
#ifdef __JS_ARCH_64_BIT
#	define	__JS_STD_PRI_PREFIX_64	"l"
#else
#	define	__JS_STD_PRI_PREFIX_64	"ll"
#endif

#ifdef __JS_BEOS_COMPATIBLE_TYPES
#	define	__JS_PRI_PREFIX_32		"l"
#else
#	define	__JS_PRI_PREFIX_32		__JS_STD_PRI_PREFIX_32
#endif
#define	__JS_PRI_PREFIX_64			__JS_STD_PRI_PREFIX_64

#define __JS_PRI_PREFIX_ADDR			"l"

#ifdef __JS_ARCH_PHYSICAL_64_BIT
#	define __JS_PRI_PREFIX_PHYS_ADDR	__JS_PRI_PREFIX_64
#else
#	define __JS_PRI_PREFIX_PHYS_ADDR	__JS_PRI_PREFIX_32
#endif


/* a generic address type wide enough for virtual and physical addresses */
#if __JS_ARCH_BITS >= __JS_ARCH_PHYSICAL_BITS
	typedef __j_addr_t					__j_generic_addr_t;
#	define __JS_GENERIC_ADDR_MAX			__JS_ADDR_MAX
#	define __JS_PRI_PREFIX_GENERIC_ADDR	__JS_PRI_PREFIX_ADDR
#else
	typedef __j_phys_addr_t				__j_generic_addr_t;
#	define __JS_GENERIC_ADDR_MAX			__JS_PHYS_ADDR_MAX
#	define __JS_PRI_PREFIX_GENERIC_ADDR	__JS_PRI_PREFIX_PHYS_ADDR
#endif


#endif	/* _J_CONFIG_TYPES_H */
