/*-
 * Copyright (c) 2018 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

.cpu cortex-m33
.syntax		unified
.thumb

.word	stack_top	/* Stack top address */
.word	app_main	/* Reset */
.word	exception	/* NMI */
.word	exception	/* Hard Fault */
.word	exception	/* mm */
.word	exception	/* Bus Fault */
.word	exception	/* Usage Fault */
.word	exception	/* Reserved */
.word	exception	/* Reserved */
.word	exception	/* Reserved */
.word	exception	/* Reserved */
.word	exception	/* SVCall */
.word	exception	/* Reserved */
.word	exception	/* Reserved */
.word	exception	/* Pend */
.word	exception	/* Systick */
.word	exception	/* IRQ 0 */
.word	exception	/* IRQ 1 */
.word	exception	/* IRQ 2 */
.word	exception	/* IRQ 3 */
.word	exception	/* IRQ 4 */
.word	exception	/* IRQ 5 */
.word	exception	/* IRQ 6 */
.word	exception	/* IRQ 7 */
.word	exception	/* IRQ 8 */
.word	exception	/* IRQ 9 */
.word	exception	/* IRQ 10 */
.word	exception	/* IRQ 11 */
.word	exception	/* IRQ 12 */
.word	exception	/* IRQ 13 */
.word	exception	/* IRQ 14 */
.word	exception	/* IRQ 15 */
.word	exception	/* IRQ 16 */
.word	exception	/* IRQ 17 */
.word	exception	/* IRQ 18 */
.word	exception	/* IRQ 19 */
.word	exception	/* IRQ 20 */
.word	exception	/* IRQ 21 */
.word	exception	/* IRQ 22 */
.word	exception	/* IRQ 23 */
.word	exception	/* IRQ 24 */
.word	exception	/* IRQ 25 */
.word	exception	/* IRQ 26 */
.word	exception	/* IRQ 27 */
.word	exception	/* IRQ 28 */
.word	exception	/* IRQ 29 */
.word	exception	/* IRQ 30 */
.word	exception	/* IRQ 31 */
.word	exception	/* IRQ 32 */
.word	exception	/* IRQ 33 */
.word	exception	/* IRQ 34 */
.word	exception	/* IRQ 35 */
.word	exception	/* IRQ 36 */
.word	exception	/* IRQ 37 */
.word	exception	/* IRQ 38 */
.word	exception	/* IRQ 39 */
.word	exception	/* IRQ 40 */
.word	exception	/* IRQ 41 */
.word	exception	/* IRQ 42 */
.word	exception	/* IRQ 43 */
.word	exception	/* IRQ 44 */
.word	exception	/* IRQ 45 */
.word	exception	/* IRQ 46 */
.word	exception	/* IRQ 47 */
.word	exception	/* IRQ 48 */
.word	exception	/* IRQ 49 */
.word	exception	/* IRQ 50 */
.word	exception	/* IRQ 51 */
.word	exception	/* IRQ 52 */
.word	exception	/* IRQ 53 */
.word	exception	/* IRQ 54 */
.word	exception	/* IRQ 55 */
.word	exception	/* IRQ 56 */
.word	exception	/* IRQ 57 */
.word	exception	/* IRQ 58 */
.word	exception	/* IRQ 59 */
.word	exception	/* IRQ 60 */
.word	exception	/* IRQ 61 */
.word	exception	/* IRQ 62 */
.word	exception	/* IRQ 63 */
.word	exception	/* IRQ 64 */
.word	exception	/* IRQ 65 */
.word	exception	/* IRQ 66 */
.word	exception	/* IRQ 67 */
.word	exception	/* IRQ 68 */
.word	exception	/* IRQ 69 */
.word	exception	/* IRQ 70 */
.word	exception	/* IRQ 71 */
.word	exception	/* IRQ 72 */
.word	exception	/* IRQ 73 */
.word	exception	/* IRQ 74 */
.word	exception	/* IRQ 75 */
.word	exception	/* IRQ 76 */
.word	exception	/* IRQ 77 */
.word	exception	/* IRQ 78 */
.word	exception	/* IRQ 79 */
.word	exception	/* IRQ 80 */
.word	exception	/* IRQ 81 */
.word	exception	/* IRQ 82 */
.word	exception	/* IRQ 83 */
.word	exception	/* IRQ 84 */
.word	exception	/* IRQ 85 */
.word	exception	/* IRQ 86 */
.word	exception	/* IRQ 87 */
.word	exception	/* IRQ 88 */
.word	exception	/* IRQ 89 */
.word	exception	/* IRQ 90 */
.word	exception	/* IRQ 91 */
.word	exception	/* IRQ 92 */
.word	exception	/* IRQ 93 */
.word	exception	/* IRQ 94 */
.word	exception	/* IRQ 95 */
.word	exception	/* IRQ 96 */
.word	exception	/* IRQ 97 */
.word	exception	/* IRQ 98 */
.word	exception	/* IRQ 99 */
.word	exception	/* IRQ 100 */
.word	exception	/* IRQ 101 */
.word	exception	/* IRQ 102 */
.word	exception	/* IRQ 103 */
.word	exception	/* IRQ 104 */
.word	exception	/* IRQ 105 */
.word	exception	/* IRQ 106 */
.word	exception	/* IRQ 107 */
.word	exception	/* IRQ 108 */
.word	exception	/* IRQ 109 */
.word	exception	/* IRQ 110 */
.word	exception	/* IRQ 111 */
.word	exception	/* IRQ 112 */
.word	exception	/* IRQ 113 */
.word	exception	/* IRQ 114 */
.word	exception	/* IRQ 115 */
.word	exception	/* IRQ 116 */
.word	exception	/* IRQ 117 */
.word	exception	/* IRQ 118 */
.word	exception	/* IRQ 119 */
.word	exception	/* IRQ 120 */
.word	exception	/* IRQ 121 */
.word	exception	/* IRQ 122 */
.word	exception	/* IRQ 123 */
.word	exception	/* IRQ 124 */
.word	exception	/* IRQ 125 */
.word	exception	/* IRQ 126 */
.word	exception	/* IRQ 127 */
