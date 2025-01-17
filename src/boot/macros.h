//********************************************************************
//	created:	6:11:2009   15:28
//	filename: 	AcpiUtils.h
//	author:		tiamo
//	purpose:	acpi utils
//********************************************************************

#ifndef __ASSERT_H__
#define __ASSERT_H__

#ifndef CAST_INTEGER
#if (defined(__i386__) || defined(__arm__) || defined(__ppc__)) /* 32 Bit */
#define CAST_INTEGER(value) ((UINT32)((unsigned int)(value)))
#else /* __LP64__ || __x86_64__ || __ppc64__ || __arm64__ || __aarch64__ */ /* 64 bit */
#define CAST_INTEGER(value) ((UINT64)((unsigned long long)(value)))
#endif /* 32/64 bit */
#endif /* CAST_INTEGER */

#if defined(__GNUC__) || defined(__clang__)
#define __ASSERT_FILE_NAME __PRETTY_FILE__
#else /* !__GNUC__ && !__clang__ */
#define __ASSERT_FILE_NAME __FILE__
#endif /* __GNUC__ || __clang__ */

/* Assert Functions */
#ifndef assert
#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__GNUC__) || defined(__clang__)
#define assert(e) extern char __ASSERT_FILE_NAME ## __LINE__ ##  _assert[(e) ? 1 : -1]
#else /* Unknown compiler */
#define assert(e) (0)
#endif /* Compiler */
#endif /* __DARWIN_UNIX03 */

#ifndef ASSERT_TRUE
#define ASSERT_TRUE(c)  assert((c) == 1)
#endif /* ASSERT_TRUE */

#ifndef ASSERT_FALSE
#define ASSERT_FALSE(c) assert(!(c) == 1)
#endif /* ASSERT_FALSE */

#ifndef GNUPACK
#if (defined(__GNUC__) || defined(__clang__))  && !defined(USE_ATTRIB_PACKED)
#define GNUPACK __attribute__((__aligned__(1)))
#elif (defined(__GNUC__) || defined(__clang__)) && defined(USE_ATTRIB_PACKED)
#define GNUPACK __attribute__((__packed__))
#elif defined(_MSC_VER) || defined(__BORLANDC__)
#define GNUPACK __declspec(align(1))
#else /* !__GNUC___ && !__clang__ && !_MSC_VER && !__BORLANDC__ */
#define GNUPACK /* XXX */
#endif /* __GNUC__ || __clang__ || _MSC_VER || __BORLANDC__ */
#endif /* GNUPACK */

#endif /* __ASSERT_H__ */
