/* Copyright (C) 2012 tueidj. SPDX-License-Identifier: LGPL-2.0-or-later
 * Entry/return layout follows vm/dsihandler.s.
 * libogc2's vector saves r0-r5, CR/LR/CTR/XER/SRR0/SRR1, but not FP state.
 */
#if defined(HW_RVL) && defined(PERF_PROF) && defined(PERF_HPROF)
#include <ppc-asm.h>
#include <ogc/machine/asm.h>
FUNC_START(hprof_entry)
    stwu sp,-EXCEPTION_FRAME_END(sp)
    stw r6,GPR6_OFFSET(sp)
    stw r7,GPR7_OFFSET(sp)
    stw r8,GPR8_OFFSET(sp)
    stw r9,GPR9_OFFSET(sp)
    stw r10,GPR10_OFFSET(sp)
    stw r11,GPR11_OFFSET(sp)
    stw r12,GPR12_OFFSET(sp)
    lwz r3,SRR0_OFFSET(sp)
    bl hprof_sample
    lwz r6,GPR6_OFFSET(sp)
    lwz r7,GPR7_OFFSET(sp)
    lwz r8,GPR8_OFFSET(sp)
    lwz r9,GPR9_OFFSET(sp)
    lwz r10,GPR10_OFFSET(sp)
    lwz r11,GPR11_OFFSET(sp)
    lwz r12,GPR12_OFFSET(sp)
    lwz r3,CR_OFFSET(sp)
    lwz r4,LR_OFFSET(sp)
    lwz r5,CTR_OFFSET(sp)
    lwz r0,XER_OFFSET(sp)
    mtcr r3
    mtlr r4
    mtctr r5
    mtxer r0
    lwz r0,GPR0_OFFSET(sp)
    lwz r5,GPR5_OFFSET(sp)
    lwz r3,SRR0_OFFSET(sp)
    lwz r4,SRR1_OFFSET(sp)
    mtsrr0 r3
    mtsrr1 r4
    lwz r3,GPR3_OFFSET(sp)
    lwz r4,GPR4_OFFSET(sp)
    lwz sp,GPR1_OFFSET(sp)
    rfi
FUNC_END(hprof_entry)
#endif
