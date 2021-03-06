/*
 * cache.h
 *
 *  Created on: Sep 16, 2014
 *      Author: lloyd23
 */

#ifndef CACHE_H_
#define CACHE_H_

//------------------ Cache ------------------//
// cache management on host and accelerator

/* NOTE: if invalidate is used on non-cache aligned and sized allocations, */
/* it can corrupt the heap. */

#ifdef __cplusplus
#define ns(name) name::
#else
#define ns(name)
#endif

#if defined(CLIENT)
#define CACHE_BARRIER(a) {a.cache_flush(); a.cache_invalidate(); ns(host)cache_flush_invalidate();}
#define CACHE_DISPOSE(a,p,n) {a.cache_invalidate(p,n); ns(host)cache_invalidate(p,n);}
#define CACHE_BARRIER2(a,b) {a.cache_flush(); a.cache_invalidate(); b.cache_flush(); b.cache_invalidate(); ns(host)cache_flush_invalidate();}
#define CACHE_DISPOSE2(a,b,p,n) {a.cache_invalidate(p,n); b.cache_invalidate(p,n); ns(host)cache_invalidate(p,n);}

#else // not CLIENT
#define CACHE_BARRIER(a) {ns(host)cache_flush_invalidate();}
#define CACHE_DISPOSE(a,p,n) {ns(host)cache_invalidate(p,n);}
#define CACHE_BARRIER2(a,b) {ns(host)cache_flush_invalidate();}
#define CACHE_DISPOSE2(a,b,p,n) {ns(host)cache_invalidate(p,n);}
#endif // end CLIENT

#if defined(USE_STREAM)
/* Not enabled with defined(DIRECT) */
#define CACHE_SEND_ALL(a) {ns(host)cache_flush(); /*a.cache_flush(); a.cache_invalidate();*/}
#define CACHE_RECV_ALL(a) {/*a.cache_flush();*/ ns(host)cache_flush_invalidate();}
#define CACHE_SEND(a,p,n) {ns(host)cache_flush(p,n); /*a.cache_invalidate(p,n);*/}
#define CACHE_RECV(a,p,n) {/*a.cache_flush(p,n);*/ ns(host)cache_invalidate(p,n);}

#else // not USE_STREAM
#define CACHE_SEND_ALL(a)
#define CACHE_RECV_ALL(a)
#define CACHE_SEND(a,p,n)
#define CACHE_RECV(a,p,n)
#endif // end USE_STREAM

#if defined(XCACHE)
#include "xpseudo_asm.h" // mtcp*, dsb
#include "xil_cache.h" // Xil_D*

#if defined(__aarch64__)
#define Xil_L1DCacheFlush Xil_DCacheFlush
#define Xil_L1DCacheFlushRange Xil_DCacheFlushRange
#define Xil_L1DCacheInvalidate Xil_DCacheInvalidate
#define Xil_L1DCacheInvalidateRange Xil_DCacheInvalidateRange
#define dc_CVAC(va) mtcpdc(CVAC,(INTPTR)(va))
#define dc_CIVAC(va) mtcpdc(CIVAC,(INTPTR)(va))
#else // not __aarch64__
#include "xil_cache_l.h" // Xil_L1D*
#define dc_CVAC(va) mtcp(XREG_CP15_CLEAN_DC_LINE_MVA_POC,(INTPTR)(va))
#define dc_CIVAC(va) mtcp(XREG_CP15_CLEAN_INVAL_DC_LINE_MVA_POC,(INTPTR)(va))
#endif // end __aarch64__

#elif defined(__microblaze__)
#include "xil_cache.h" // Xil_L1D*, Xil_D*

#else // not XCACHE, __microblaze__
/* Data Synchronization Barrier */
#define dsb()

/* single cache line */
#define dc_CVAC(va)
#define dc_CIVAC(va)

/* L1 data cache */
#define Xil_L1DCacheEnable()
#define Xil_L1DCacheDisable()
#define Xil_L1DCacheInvalidate()
#define Xil_L1DCacheInvalidateRange(adr, len)
#define Xil_L1DCacheInvalidateLine(adr)
#define Xil_L1DCacheFlush()
#define Xil_L1DCacheFlushRange(adr, len)
#define Xil_L1DCacheFlushLine(adr)
#define Xil_L1DCacheStoreLine(adr)

#define Xil_DCacheEnable()
#define Xil_DCacheDisable()
#define Xil_DCacheInvalidate()
#define Xil_DCacheInvalidateRange(adr, len)
#define Xil_DCacheInvalidateLine(adr)
#define Xil_DCacheFlush()
#define Xil_DCacheFlushRange(adr, len)
#define Xil_DCacheFlushLine(adr)

#define Xil_ICacheEnable()
#define Xil_ICacheDisable()
#define Xil_ICacheInvalidate()
#define Xil_ICacheInvalidateRange(adr, len)
#define Xil_ICacheInvalidateLine(adr)

#define Xil_ConfigureL1Prefetch(num)

#endif // end XCACHE, __microblaze__

#ifdef __cplusplus
#if defined(SYSTEMC)
#include <systemc.h>
#endif
namespace host {

#if defined(ZYNQ)
#if defined(__aarch64__)
inline void cache_flush(void) {Xil_DCacheFlush();}
inline void cache_flush(const void *ptr, size_t size) {Xil_DCacheFlushRange((INTPTR)ptr, size);}
inline void cache_flush_invalidate(void) {Xil_DCacheFlush();}
inline void cache_flush_invalidate(const void *ptr, size_t size) {Xil_DCacheFlushRange((INTPTR)ptr, size);}
inline void cache_invalidate(void) {Xil_DCacheInvalidate();}
inline void cache_invalidate(const void *ptr, size_t size) {Xil_DCacheInvalidateRange((INTPTR)ptr, size);}

#else // not __aarch64__
// Differentiate between scratchpad (SP) with only L1 enabled (Xil_L1DCache*)
// and DRAM space with both L1 and L2 enabled (Xil_DCache*).
#define IS_SP(ptr) ((char*)(ptr) >= (char*)0x40000000 && (char*)(ptr) < (char*)0x40200000)
inline void cache_flush(void) {Xil_DCacheFlush();}
inline void cache_flush(const void *ptr, size_t size)
{
	if (IS_SP(ptr)) Xil_L1DCacheFlushRange((INTPTR)ptr, size);
	else Xil_DCacheFlushRange((INTPTR)ptr, size);
}
inline void cache_flush_invalidate(void) {Xil_DCacheFlush();}
inline void cache_flush_invalidate(const void *ptr, size_t size)
{
	if (IS_SP(ptr)) Xil_L1DCacheFlushRange((INTPTR)ptr, size);
	else Xil_DCacheFlushRange((INTPTR)ptr, size);
}
inline void cache_invalidate(void) {Xil_DCacheInvalidate();}
inline void cache_invalidate(const void *ptr, size_t size)
{
	if (IS_SP(ptr)) Xil_L1DCacheInvalidateRange((INTPTR)ptr, size);
	else Xil_DCacheInvalidateRange((INTPTR)ptr, size);
}
#endif // end __aarch64__

#elif defined(SYSTEMC)
// cache management overhead in ns per byte
#define _NSPB .230
inline void cache_flush(void) {}
inline void cache_flush(const void *ptr, size_t size) {wait(_NSPB*size,SC_NS);}
inline void cache_flush_invalidate(void) {}
inline void cache_flush_invalidate(const void *ptr, size_t size) {wait(_NSPB*size,SC_NS);}
inline void cache_invalidate(void) {}
inline void cache_invalidate(const void *ptr, size_t size) {wait(_NSPB*size,SC_NS);}
#undef _NSPB

#else // not ZYNQ, SYSTEMC
inline void cache_flush(void) {}
inline void cache_flush(const void *ptr, size_t size) {}
inline void cache_flush_invalidate(void) {}
inline void cache_flush_invalidate(const void *ptr, size_t size) {}
inline void cache_invalidate(void) {}
inline void cache_invalidate(const void *ptr, size_t size) {}
#endif // end ZYNQ, SYSTEMC

} // namespace host
#else // not __cplusplus

static inline void cache_flush(void) {Xil_DCacheFlush();}
static inline void cache_flush_invalidate(void) {Xil_DCacheFlush();}

#endif // end __cplusplus

#endif /* end CACHE_H_ */
