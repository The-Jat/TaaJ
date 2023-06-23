#ifndef _J_CONFIG_JS_CONFIG_H
#define _J_CONFIG_JS_CONFIG_H


/* Determine the architecture and define macros for some fundamental
   properties:
   __JS_ARCH					- short name of the architecture (used in paths)
   __JS_ARCH_ABI				- name of ABI (as in package architecture)
   __JS_ARCH_<arch>			- defined to 1 for the respective architecture
   __JS_ARCH_BITS			- defined to 32/64 on 32/64 bit architectures
   								  (defaults to 32)
   __JS_ARCH_PHYSICAL_BITS	- defined to 32/64 on architectures with 32/64
   								  (defaults to __JS_ARCH_BITS)
   __JS_BIG_ENDIAN			- defined to 1 on big endian architectures
   								  (defaults to undefined)
*/
#if defined(__i386__)
#	define __JS_ARCH					x86
#	if __GNUC__ == 2
#		define __JS_ARCH_ABI			"x86_gcc2"
#	else
#		define __JS_ARCH_ABI			"x86"
#	endif
#	define __JS_ARCH_X86				1
#	define __JS_ARCH_PHYSICAL_BITS	64
#elif defined(__x86_64__)
#	define __JS_ARCH					x86_64
#	define __JS_ARCH_ABI				"x86_64"
#	define __JS_ARCH_X86_64			1
#	define __JS_ARCH_BITS			64
#elif defined(__POWERPC__)
#	define __JS_ARCH					ppc
#	define __JS_ARCH_ABI				"ppc"
#	define __JS_ARCH_PPC				1
#	define __JS_ARCH_PHYSICAL_BITS	64
#	define __JS_BIG_ENDIAN			1
#elif defined(__M68K__)
#	define __JS_ARCH					m68k
#	define __JS_ARCH_ABI				"m68k"
#	define __JS_ARCH_M68K			1
#	define __JS_BIG_ENDIAN			1
#elif defined(__MIPSEL__)
#	define __JS_ARCH					mipsel
#	define __JS_ARCH_ABI				"mipsel"
#	define __JS_ARCH_MIPSEL			1
#elif defined(__ARMEL__) || defined(__arm__)
#	define __JS_ARCH					arm
#	define __JS_ARCH_ABI				"arm"
#	define __JS_ARCH_ARM				1
#elif defined(__ARMEB__)
#	define __JS_ARCH					armeb
#	define __JS_ARCH_ABI				"armeb"
#	define __JS_ARCH_ARM				1
#	define __JS_BIG_ENDIAN			1
#elif defined(__aarch64__) || defined(__arm64__)
#	define __JS_ARCH					arm64
#	define __JS_ARCH_ABI				"arm64"
#	define __JS_ARCH_ARM64			1
#	define __JS_ARCH_BITS			64
#elif defined(__riscv64__) || (defined(__riscv) && __riscv_xlen == 64)
#	define __JS_ARCH					riscv64
#	define __JS_ARCH_ABI				"riscv64"
#	define __JS_ARCH_RISCV64			1
#	define __JS_ARCH_BITS			64
#elif defined(__sparc64__)
#	define __JS_ARCH					sparc64
#	define __JS_ARCH_ABI				"sparc"
#	define __JS_ARCH_SPARC			1
#	define __JS_ARCH_PHYSICAL_BITS	64
#	define __JS_BIG_ENDIAN			1
#	define __JS_ARCH_BITS			64
#else
#	error Unsupported architecture!
#endif

/* implied properties:
   __JS_ARCH_{32,64}_BIT		- defined to 1 on 32/64 bit architectures, i.e.
   								  using 32/64 bit virtual addresses
   __JS_ARCH_PHYSICAL_BITS	- defined to 32/64 on architectures with 32/64
   								  bit physical addresses
   __JS_ARCH_PHYSICAL_{32,64}_BIT - defined to 1 on architectures using 64
   								  bit physical addresses
   __JS_BIG_ENDIAN			- defined to 1 on big endian architectures
*/

/* bitness */
#ifndef __JS_ARCH_BITS
#	define __JS_ARCH_BITS		32
#endif

#if __JS_ARCH_BITS == 32
#	define __JS_ARCH_32_BIT		1
#elif __JS_ARCH_BITS == 64
#	define __JS_ARCH_64_BIT		1
#else
#	error Unsupported bitness!
#endif

/* physical bitness */
#ifndef __JS_ARCH_PHYSICAL_BITS
#	define __JS_ARCH_PHYSICAL_BITS	__JS_ARCH_BITS
#endif

#if __JS_ARCH_PHYSICAL_BITS == 32
#	define __JS_ARCH_PHYSICAL_32_BIT	1
#elif __JS_ARCH_PHYSICAL_BITS == 64
#	define __JS_ARCH_PHYSICAL_64_BIT	1
#else
#	error Unsupported physical bitness!
#endif

/* endianess */
#ifndef __JS_BIG_ENDIAN
#	define	__JS_LITTLE_ENDIAN	1
#endif

/* architecture specific include macros */
#define __JS_ARCH_HEADER(header)					<arch/__JS_ARCH/header>
#define __JS_SUBDIR_ARCH_HEADER(subdir, header)	\
	<subdir/arch/__JS_ARCH/header>

/* BeOS R5 binary compatibility (gcc 2 on x86) */
#if defined(__JS_ARCH_X86) && __GNUC__ == 2
#	define __JS_BEOS_COMPATIBLE	1
#endif

/* BeOS R5 compatible types */
#if defined(__JS_ARCH_X86)
	/* TODO: This should be "#ifdef __JS_BEOS_COMPATIBLE", but this will
	   break all gcc 4 C++ optional packages. I.e. switch that at a suitable
	   time.
	*/
#	define __JS_BEOS_COMPATIBLE_TYPES	1
#endif


#endif	/* _J_CONFIG_JS_CONFIG_H */
