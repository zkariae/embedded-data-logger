  .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

.global  g_pfnVectors
.global  Default_Handler

.word  _sidata
.word  _sdata
.word  _edata
.word  _sbss
.word  _ebss

  .section  .text.Reset_Handler
  .weak  Reset_Handler
  .type  Reset_Handler, %function
Reset_Handler:
  movs  r1, #0
  b  LoopCopyDataInit

CopyDataInit:
  ldr  r3, =_sidata
  ldr  r3, [r3, r1]
  str  r3, [r0, r1]
  adds  r1, r1, #4

LoopCopyDataInit:
  ldr  r0, =_sdata
  ldr  r3, =_edata
  adds  r2, r0, r1
  cmp  r2, r3
  bcc  CopyDataInit
  ldr  r2, =_sbss
  b  LoopFillZerobss

FillZerobss:
  movs  r3, #0
  str  r3, [r2], #4

LoopFillZerobss:
  ldr  r3, =_ebss
  cmp  r2, r3
  bcc  FillZerobss

  bl  SystemInit
  bl  main
  bx  lr
  .size  Reset_Handler, .-Reset_Handler

  .section  .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b  Infinite_Loop
  .size  Default_Handler, .-Default_Handler

  .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object

g_pfnVectors:
  .word  _estack
  .word  Reset_Handler       @ Reset
  .word  Default_Handler     @ NMI
  .word  HardFault_Handler   @ HardFault
  .word  Default_Handler     @ MemManage
  .word  Default_Handler     @ BusFault
  .word  Default_Handler     @ UsageFault
  .word  0                   @ Reserved
  .word  0                   @ Reserved
  .word  0                   @ Reserved
  .word  0                   @ Reserved
  .word  Default_Handler     @ SVC
  .word  Default_Handler     @ DebugMon
  .word  0                   @ Reserved
  .word  Default_Handler     @ PendSV
  .word  SysTick_Handler     @ SysTick
  /* External Interrupts */
  .word  Default_Handler     @ WWDG
  .word  Default_Handler     @ PVD
  .word  Default_Handler     @ TAMP_STAMP
  .word  Default_Handler     @ RTC_WKUP
  .word  Default_Handler     @ FLASH
  .word  Default_Handler     @ RCC
  .word  EXTI0_IRQHandler    @ EXTI0  <- Bouton USER PA0
  .word  Default_Handler     @ EXTI1
  .word  Default_Handler     @ EXTI2
  .word  Default_Handler     @ EXTI3
  .word  Default_Handler     @ EXTI4

  .weak  NMI_Handler
  .thumb_set NMI_Handler,Default_Handler
  .weak  HardFault_Handler
  .thumb_set HardFault_Handler,Default_Handler
  .weak  SysTick_Handler
  .thumb_set SysTick_Handler,Default_Handler
  .weak  EXTI0_IRQHandler
  .thumb_set EXTI0_IRQHandler,Default_Handler
