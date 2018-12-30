/*
 * Copyright (c) 2005, National ICT Australia (NICTA)
 */
/*
 * Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
 * All rights reserved.
 *
 * 1. Redistribution and use of OKL4 (Software) in source and binary
 * forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 *     (a) Redistributions of source code must retain this clause 1
 *         (including paragraphs (a), (b) and (c)), clause 2 and clause 3
 *         (Licence Terms) and the above copyright notice.
 *
 *     (b) Redistributions in binary form must reproduce the above
 *         copyright notice and the Licence Terms in the documentation and/or
 *         other materials provided with the distribution.
 *
 *     (c) Redistributions in any form must be accompanied by information on
 *         how to obtain complete source code for:
 *        (i) the Software; and
 *        (ii) all accompanying software that uses (or is intended to
 *        use) the Software whether directly or indirectly.  Such source
 *        code must:
 *        (iii) either be included in the distribution or be available
 *        for no more than the cost of distribution plus a nominal fee;
 *        and
 *        (iv) be licensed by each relevant holder of copyright under
 *        either the Licence Terms (with an appropriate copyright notice)
 *        or the terms of a licence which is approved by the Open Source
 *        Initative.  For an executable file, "complete source code"
 *        means the source code for all modules it contains and includes
 *        associated build and other files reasonably required to produce
 *        the executable.
 *
 * 2. THIS SOFTWARE IS PROVIDED ``AS IS'' AND, TO THE EXTENT PERMITTED BY
 * LAW, ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, OR NON-INFRINGEMENT, ARE DISCLAIMED.  WHERE ANY WARRANTY IS
 * IMPLIED AND IS PREVENTED BY LAW FROM BEING DISCLAIMED THEN TO THE
 * EXTENT PERMISSIBLE BY LAW: (A) THE WARRANTY IS READ DOWN IN FAVOUR OF
 * THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
 * PARTICIPANT) AND (B) ANY LIMITATIONS PERMITTED BY LAW (INCLUDING AS TO
 * THE EXTENT OF THE WARRANTY AND THE REMEDIES AVAILABLE IN THE EVENT OF
 * BREACH) ARE DEEMED PART OF THIS LICENCE IN A FORM MOST FAVOURABLE TO
 * THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
 * PARTICIPANT). IN THE LICENCE TERMS, "PARTICIPANT" INCLUDES EVERY
 * PERSON WHO HAS CONTRIBUTED TO THE SOFTWARE OR WHO HAS BEEN INVOLVED IN
 * THE DISTRIBUTION OR DISSEMINATION OF THE SOFTWARE.
 *
 * 3. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ANY OTHER PARTICIPANT BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Author: Philip O'Sullivan <philip.osullivan@nicta.com.au>
 * Date  : Thu 10 Nov 2005
 */

#include <mutex/mutex.h>
#include <l4/thread.h>

static int try_lock(okl4_mutex_t m, L4_Word_t me);

void
okl4_mutex_lock(okl4_mutex_t m)
{
    L4_Word_t me = L4_Myself().raw;
    L4_ThreadId_t holder;

    holder.raw = m->holder;
    while (!try_lock(m, me)) {
        L4_ThreadSwitch(holder);
    }
}

int
okl4_mutex_trylock(okl4_mutex_t m)
{
    L4_Word_t me = L4_Myself().raw;

    return !try_lock(m, me);
}

static int
try_lock(okl4_mutex_t m, L4_Word_t me)
{
       volatile L4_Word_t temp = 0, me_reg = me;

#if defined(L4_32BIT)
       __asm__ __volatile__("  .set    noreorder       \n"
                            "  ll      %2, %0          \n"
                            "  beq     %2, $0, 1f      \n"
                            "  nop                     \n"
                            "  bne     %2, %1, 2f      \n"
                            "1: add    %3, %1, $0      \n"
                            "  sc      %3, %0          \n"
                            "2: nop                    \n"
                            "  .set    reorder         \n"
                            :  "=m" ((m)->holder), "+r" (me) : "r" (temp), "r" (me_reg)
           );
#elif defined(L4_64BIT)
       __asm__ __volatile__("  .set    noreorder       \n"
                            "  lld     %2, %0          \n"
                            "  beq     %2, $0, 1f      \n"
                            "  nop                     \n"
                            "  bne     %2, %1, 2f      \n"
                            "1: dadd   %3, %1, $0      \n"
                            "  scd     %3, %0          \n"
                            "2: nop                    \n"
                            "  .set    reorder         \n"
                            :  "=m" ((m)->holder), "+r" (me) : "r" (temp), "r" (me_reg)
           );
#endif
       return m->holder == me;
}
