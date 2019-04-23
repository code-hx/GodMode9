/*
 *   This file is part of GodMode9
 *   Copyright (C) 2017-2019 Wolfvak
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

.section .text.boot
.align 4

#include <arm.h>

#define STACK_SZ    (16384)

.global __boot
__boot:
	cpsid aif, #SR_SVC_MODE

    @ Writeback and invalidate all DCache
    @ Invalidate all caches
    @ Data Synchronization Barrier
    mov r0, #0
    mcr p15, 0, r0, c7, c10, 0
    mcr p15, 0, r0, c7, c7, 0
    mcr p15, 0, r0, c7, c10, 4

    @ Reset control registers
    ldr r0, =0x00054078
    ldr r1, =0x0000000F
    ldr r2, =0x00F00000

    mcr p15, 0, r0, c1, c0, 0
    mcr p15, 0, r1, c1, c0, 1
    mcr p15, 0, r2, c1, c0, 2

    @ Get CPU ID
    mrc p15, 0, r12, c0, c0, 5
    ands r12, r12, #3

    @ Setup stack according to CPU ID
    ldr sp, =(_stack_base + STACK_SZ)
    ldr r0, =STACK_SZ
    mla sp, r0, r12, sp

    beq corezero_start

    cmp r12, #MAX_CPU
    blo coresmp_start

1:
    wfi
    b 1b

corezero_start:
    @ assume __bss_len is 128 byte aligned
    ldr r0, =__bss_pa
    ldr r1, =__bss_len
    add r1, r0, r1
    mov r2, #0
    mov r3, #0
    mov r4, #0
    mov r5, #0
    .Lclearbss:
        cmp r0, r1
        .rept (128 / 16) @ 16 bytes copied per block
        stmloia r0!, {r2-r5}
        .endr
        blo .Lclearbss

    bl SYS_CoreZeroInit

coresmp_start:
    bl SYS_CoreInit
    b MainLoop

.section .bss.stack
.align 3
_stack_base:
    .space (MAX_CPU * STACK_SZ)
