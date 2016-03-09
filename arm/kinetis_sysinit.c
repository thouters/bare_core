/*
 *    kinetis_sysinit.c - Default init routines for P0
 *                     		Kinetis ARM systems
 *    Copyright � 2012 Freescale semiConductor Inc. All Rights Reserved.
 */

#include "kinetis_sysinit.h"
#include <stdint.h>


void Default_Handler(void);


/**
 **===========================================================================
 **  External declarations
 **===========================================================================
 */
#if __cplusplus
extern "C" {
#endif

extern void __thumb_startup(void);
extern int _end_stack[];

#if __cplusplus
}
#endif

/**
 **===========================================================================
 **  Default interrupt handler
 **===========================================================================
 */
void Default_Handler()
{
	while (1)
		;
}



/* Vector table for FB200 */

/* The Interrupt Vector Table */
void (* const InterruptVector[])() __attribute__ ((section(".vectortable"))) = {
    /* Processor exceptions */
    (void(*)(void)) &_end_stack,   // 0
    __thumb_startup,    // 1 Reset
    Default_Handler,    // 2 NMI
    Default_Handler,  // 3 Hard fault
    Default_Handler,    // 4 MemManage
    Default_Handler,    // 5 Bus Fault
    Default_Handler,    // 6 Usage Fault
    0,                  // 7 - 10 Reserved
    0,
    0,
    0,
    Default_Handler,    // 11 SVCall
    Default_Handler,    // 12 Debug
    0,                  // 13
    Default_Handler,    // 14 PendSV
    Default_Handler,    // 15 SysTick

    /* Interrupts */
    Default_Handler,    // 0
    Default_Handler,    // 1
    Default_Handler,    // 2
    Default_Handler,    // 3
    Default_Handler,    // 4
    Default_Handler,    // 5
    Default_Handler,    // 6
    Default_Handler,    // 7
    Default_Handler,    // 8
    Default_Handler,    // 9
    Default_Handler,    // 10
    Default_Handler,    // 11
    Default_Handler,    // 12
    Default_Handler,    // 13
    Default_Handler,    // 14
    Default_Handler,    // 15
    Default_Handler,    // 16
    Default_Handler,    // 17
    Default_Handler,    // 18
    Default_Handler,    // 19
    Default_Handler,    // 20
    Default_Handler,    // 21
    Default_Handler,    // 22
    Default_Handler,    // 23
    Default_Handler,    // 24
    Default_Handler,    // 25
    Default_Handler,    // 26
    Default_Handler,    // 27
    Default_Handler,    // 28
    Default_Handler,    // 29
    Default_Handler,    // 30
    Default_Handler,   // 31
    Default_Handler,    // 32
    Default_Handler,    // 33
    Default_Handler,    // 34
    Default_Handler,       // 35
    Default_Handler,    // 36
    Default_Handler,    // 37
    Default_Handler,    // 38
    Default_Handler,    // 39
    Default_Handler,    // 40
    Default_Handler,    // 41
    Default_Handler,         // 42
    Default_Handler,         // 43
    Default_Handler,         // 44
    Default_Handler,    // 45
    Default_Handler,    // 46
    Default_Handler,    // 47
    Default_Handler,    // 48
    Default_Handler,    // 49
    Default_Handler,    // 50
    Default_Handler,    // 51
    Default_Handler,    // 52
    Default_Handler,    // 53
    Default_Handler,    // 54
    Default_Handler,    // 55
    Default_Handler,    // 56
    Default_Handler,    // 57
    Default_Handler,    // 58
    Default_Handler,        // 59 PortA
    Default_Handler,    // 60 PortB
    Default_Handler,    // 61 PortC
    Default_Handler,        // 62 PortD
    Default_Handler,   // 63 PortE
    Default_Handler,    // 64
};
