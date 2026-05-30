#include <stdbool.h>
#include "s32k1xx.h"


struct pin {
  char port;
  int num;
};

struct pin pins[] = {

  // J12
  {'E', 9},
  {'D', 15},
  {'D', 16},
  {'E', 3},
  {'E', 4},
  {'E', 5},
  {'E', 10},
  {'E', 11},
  {'D', 0},
  {'D', 1},

  // J16
  {'B', 5},
  {'E', 8},
  {'C', 3},
  {'B', 4},
  {'D', 7},
  {'C', 2},
  {'D', 5},
  {'D', 6},
  {'C', 0},
  {'C', 1},
  {'C', 16},
  {'C', 17},
  {'C', 14},
  {'C', 15},
  {'B', 2},
  {'B', 3},

  // J15
 // {'A', 5},  RST
//  {'A', 4}, SWDIO
  {'C', 5},  // held high?
//  {'C', 4}, SWCLK
  {'E', 1},
  {'E', 0},
  {'A', 11},
  {'A', 10}, //not working
  {'A', 13},
  {'A', 12},
  {'E', 6},
  {'E', 2},
  {'C', 7},
  {'C', 6},
  {'A', 1},
  {'A', 0},

  // J13
  {'B', 1},
  {'B', 0},
  {'C', 9},
  {'C', 8},
  {'A', 7},
  {'A', 6},
  {'E', 7},
  {'B', 13},
  {'B', 12},
  {'D', 4},
  {'D', 3},
  {'D', 2},
  {'A', 3},
  {'A', 2},
};

const int n_pins = sizeof(pins) / sizeof(struct pin);

// Configure all pins (except SWD and (E)XTAL) as GPIO outputs
// and toggle them in a sequence
void mcxe246_hwtest_pinheaders(void) {
  *PCC_PORTA = PCC_CGC;
  *PCC_PORTB = PCC_CGC;
  *PCC_PORTC = PCC_CGC;
  *PCC_PORTD = PCC_CGC;
  *PCC_PORTE = PCC_CGC;


  for (int i = 0; i < n_pins; i++) {
    switch (pins[i].port) {
      case 'A':
        *PORTA_PCRn(pins[i].num) |= PORT_PCRn_MUX(1);
        *GPIOA_PDDR |= (1u << pins[i].num);
        break;
      case 'B':
        *PORTB_PCRn(pins[i].num) |= PORT_PCRn_MUX(1);
        *GPIOB_PDDR |= (1u << pins[i].num);
        break;
      case 'C':
        *PORTC_PCRn(pins[i].num) |= PORT_PCRn_MUX(1);
        *GPIOC_PDDR |= (1u << pins[i].num);
        break;
      case 'D':
        *PORTD_PCRn(pins[i].num) |= PORT_PCRn_MUX(1);
        *GPIOD_PDDR |= (1u << pins[i].num);
        break;
      case 'E':
        *PORTE_PCRn(pins[i].num) |= PORT_PCRn_MUX(1);
        *GPIOE_PDDR |= (1u << pins[i].num);
        break;
      default:
        break;
    }
  }

  while (true) {
  for (int i = 0; i < n_pins; i++) {
    switch (pins[i].port) {
      case 'A':
        *GPIOA_PTOR = (1u << pins[i].num);
        break;
      case 'B':
        *GPIOB_PTOR = (1u << pins[i].num);
        break;
      case 'C':
        *GPIOC_PTOR = (1u << pins[i].num);
        break;
      case 'D':
        *GPIOD_PTOR = (1u << pins[i].num);
        break;
      case 'E':
        *GPIOE_PTOR = (1u << pins[i].num);
        break;
      default:
        break;
    }
  }
  }
}

