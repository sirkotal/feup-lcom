#include <lcom/lcf.h>

#include <lcom/lab3.h>

#include <stdbool.h>
#include <stdint.h>
#include "i8042.h"
#include "keyboard.h"

#define FAIL 1
#define SUCCESS 0

int main(int argc, char *argv[]) {
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need it]
  lcf_trace_calls("/home/lcom/labs/lab3/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/lab3/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return 0;
}

uint32_t sys_count = 0;
extern uint8_t scancode;


int(kbd_test_scan)() {
  int r, ipc_status;
  message msg;
  uint8_t bit_no;

  if (KBC_subscribe_ints(&bit_no)) {
    return FAIL;
  }

  uint8_t bytes[2];
  int index = 0;

  while(scancode != ESC_BREAK_CODE) {
    /* Get a request message. */
    if ( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) { 
      printf("driver_receive failed with: %d", r);
      continue;
    }
    if (is_ipc_notify(ipc_status)) { /* received notification */
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE: /* hardware interrupt notification */				
          if (msg.m_notify.interrupts & bit_no) { /* subscribed interrupt */
            if (KBC_int_handler()) {
              return FAIL;
            }

            if (scancode == TWO_BYTE_CODE) {
              bytes[index] = scancode;
              index++;
              continue;
            }

            bytes[index] = scancode;
            bool make = !(scancode & BIT(7));

            if (kbd_print_scancode(make, index + 1, bytes)) {
              return FAIL;
            }
            
            index = 0;
          }
          break;
        default:
          break; /* no other notifications expected: do nothing */	
      }
    } else { /* received a standard message, not a notification */
        /* no standard messages expected: do nothing */
    }
  }

  if (KBC_unsubscribe_ints()) {
    return FAIL;
  }

  return kbd_print_no_sysinb(sys_count);
}

int(kbd_test_poll)() {
  uint8_t bytes[2];
  int index = 0;

  while (scancode != ESC_BREAK_CODE) {
    if (KBC_read_data(&scancode)) {
      return FAIL;
    }

    if (scancode == TWO_BYTE_CODE) {
      bytes[index] = scancode;
      index++;
      continue;
    }

    bytes[index] = scancode;
    bool make = !(scancode & BIT(7));

    if (kbd_print_scancode(make, index + 1, bytes)) {
      return FAIL;
    }

    index = 0;
  }

  if (keyboard_enable_ints()) {
    return FAIL;
  }

  return kbd_print_no_sysinb(sys_count);

}

extern int timer_int_counter;

int(kbd_test_timed_scan)(uint8_t n) {
  int r, ipc_status;
  message msg;
  uint8_t KBD_bit_no;
  uint8_t timer_bit_no;

  if (KBC_subscribe_ints(&KBD_bit_no)) {
    return FAIL;
  }

  if (timer_subscribe_int(&timer_bit_no)) {
    return FAIL;
  }

  uint8_t bytes[2];
  int index = 0;

  while(scancode != ESC_BREAK_CODE && (timer_int_counter/60) != n) {
    /* Get a request message. */
    if ( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) { 
      printf("driver_receive failed with: %d", r);
      continue;
    }
    if (is_ipc_notify(ipc_status)) { /* received notification */
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE: /* hardware interrupt notification */		
          if (msg.m_notify.interrupts & timer_bit_no) {
            timer_int_handler();
          }
          if (msg.m_notify.interrupts & KBD_bit_no) { /* subscribed interrupt */
            if (KBC_int_handler()) {
              return FAIL;
            }

            if (scancode == TWO_BYTE_CODE) {
              bytes[index] = scancode;
              index++;
              continue;
            }

            bytes[index] = scancode;
            bool make = !(scancode & BIT(7));

            if (kbd_print_scancode(make, index + 1, bytes)) {
              return FAIL;
            }
            
            index = 0;
            timer_int_counter = 0;
          }
          break;
        default:
          break; /* no other notifications expected: do nothing */	
      }
    } else { /* received a standard message, not a notification */
        /* no standard messages expected: do nothing */
    }
  }

  if (KBC_unsubscribe_ints()) {
    return FAIL;
  }

  if (timer_unsubscribe_int()) {
    return FAIL;
  }

  return kbd_print_no_sysinb(sys_count);
}
