#ifndef CORTEX_M4_H
#define CORTEX_M4_H

#include <stdint.h>

#define MEM32(X)              ((volatile uint32_t *)(X))
#define MEM16(X)              ((volatile uint16_t *)(X))
#define MEM8(X)              ((volatile uint8_t *)(X))

#define COREDEBUG_DEMCR       MEM32(0xE000EDFCul)
#define COREDEBUG_DEMCR_TRCENA (1ul << 24)

#define ITM_STIM(X)           MEM32(0xE0000000ul + 4 * (X))
#define ITM_TER(X)            MEM32(0xE0000E00ul + 4 * (X))
#define ITM_TPR               MEM32(0xE0000E40ul)
#define ITM_TCR               MEM32(0xE0000E80ul)

#define DWT_CTRL              MEM32(0xE0001000ul)
#define DWT_CYCCNT            MEM32(0xE0001004ul)
#define DWT_CPICNT            MEM32(0xE0001008ul)
#define DWT_EXCCNT            MEM32(0xE000100Cul)
#define DWT_SLEEPCNT          MEM32(0xE0001010ul)
#define DWT_LSUCNT            MEM32(0xE0001014ul)
#define DWT_FOLDCNT           MEM32(0xE0001018ul)
#define DWT_PCSR              MEM32(0xE000101Cul)
#define DWT_COMP(X)           MEM32(0xE0001020ul + 16 * (X))
#define DWT_MASK(X)           MEM32(0xE0001024ul + 16 * (X))
#define DWT_FUNCTION(X)       MEM32(0xE0001028ul + 16 * (X))

#define DWT_CTRL_CYCCNTENA    (1ul << 0)

#define TPIU_SSPSR           MEM32(0xE0040000ul)
#define TPIU_CSPSR           MEM32(0xE0040004ul)
#define TPIU_ACPR            MEM32(0xE0040010ul)
#define TPIU_SPPR            MEM32(0xE00400F0ul)


#define NVIC_ICTR            MEM32(0xE000E004ul)
#define NVIC_ISER(X)         MEM32(0xE000E100ul + 4 * (X))
#define NVIC_ICER(X)         MEM32(0xE000E180ul + 4 * (X))
#define NVIC_ISPR(X)         MEM32(0xE000E200ul + 4 * (X))
#define NVIC_ICPR(X)         MEM32(0xE000E280ul + 4 * (X))
#define NVIC_IABR(X)         MEM32(0xE000E300ul + 4 * (X))
#define NVIC_IPR(X)          MEM32(0xE000E400ul + 4 * (X))

#define SCB_CPUID            MEM32(0xE000ED00ul)
#define SCB_ICSR             MEM32(0xE000ED04ul)
#define SCB_VTOR             MEM32(0xE000ED08ul)
#define SCB_AIRCR            MEM32(0xE000ED0Cul)
#define SCB_SCR              MEM32(0xE000ED10ul)
#define SCB_CCR              MEM32(0xE000ED14ul)
#define SCB_SHPR1            MEM32(0xE000ED18ul)
#define SCB_SHPR2            MEM32(0xE000ED1Cul)
#define SCB_SHPR3            MEM32(0xE000ED20ul)
#define SCB_SHCSR            MEM32(0xE000ED24ul)
#define SCB_CFSR             MEM32(0xE000ED28ul)
#define SCB_HFSR             MEM32(0xE000ED2Cul)
#define SCB_DFSR             MEM32(0xE000ED30ul)
#define SCB_MMFAR            MEM32(0xE000ED34ul)
#define SCB_BFAR             MEM32(0xE000ED38ul)
#define SCB_AFSR             MEM32(0xE000ED3Cul)
#define SCB_CPACR            MEM32(0xE000ED88ul)

static inline void _disable_interrupts(void) {
  __asm__("cpsid i");
}

static inline void _enable_interrupts(void) {
  __asm__("cpsie i");
}

static inline void nvic_enable_irq(uint16_t irq) {
  volatile uint32_t *reg = NVIC_ISER(irq / 32);
  *reg = 1 << (irq & 0x1F);
}

static inline void nvic_disable_irq(uint16_t irq) {
  volatile uint32_t *reg = NVIC_ICER(irq / 32);
  *reg = 1 << (irq & 0x1F);
  __asm__("dsb");
  __asm__("isb");
}

static inline void nvic_set_priority(uint16_t irq, uint8_t priority) {
  volatile uint32_t *reg = NVIC_IPR(irq / 4);
  uint8_t prio = priority << 4;
  *reg = prio << ((irq & 0x3) * 8);
}

static inline void exception_set_priority(uint16_t exc, uint8_t priority) {
  volatile uint32_t *reg = SCB_SHPR1 + (exc / 4);
  uint8_t prio = priority << 4;
  *reg = prio << ((exc & 0x3) * 8);
}

#endif
