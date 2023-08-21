#include <bsp/board.h>
#include <tusb.h>

#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <hardware/irq.h>

#define BAUD_RATE 1200
#define UART_KBD_ID uart1
#define UART_KBD_TX_GPIO 4

typedef enum {
    Mode0_Compatibility,
    Mode1_Keystate,
    Mode2_RelativeCursor,
    Mode3_AbsoluteCursor
} KeyboardMode;

typedef enum {
    State_Unshifted = 0,
    State_Shifted,
    State_Control,
    StateMax
} KeyState;

static KeyboardMode kbd_mode = Mode0_Compatibility;

static void on_keyboard_rx();

// defined at end of file
static char s_code_table[256][StateMax];

void apollo_dn300_init() {
    gpio_set_function(UART_KBD_TX_GPIO, GPIO_FUNC_UART);

    uart_init(UART_KBD_ID, BAUD_RATE);
    uart_set_hw_flow(UART_KBD_ID, false, false);
    uart_set_format(UART_KBD_ID, 8, 1, UART_PARITY_NONE);
}

void apollo_dn300_update() {
}


void apollo_dn300_kbd_report(hid_keyboard_report_t const *report) {
    static bool down_state[256] = {false};

    uint8_t up_keys[6]; // up to 6 keys could have been released this frame
    uint8_t up_key_count = 0;

    uint8_t down_keys[6]; // up to 6 keys could have been pressed this frame
    uint8_t down_key_count = 0;

    KeyState selector = State_Unshifted;

    bool ctrl =  (report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL)) != 0;
    bool shift = (report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT)) != 0;
    //bool caps = 
    //bool repeat =

    // find any released keys.  this is a silly loop.
    for (int hidk = 1; hidk < 256; hidk++) {
		if (!down_state[hidk]) {
			// the key is not down currently
			for (int j = 0; j < 6; j++) {
				if (report->keycode[j] == hidk) {
					down_state[hidk] = true;
					down_keys[down_key_count++] = hidk;
					break;
				}
			}
		} else {
			// the key is down currently.  if it's not in the report, it's been released.
			bool found = false;
			for (int j = 0; j < 6; j++) {
				if (report->keycode[j] == hidk) {
					found = true;
					break;
				}
			}
			if (!found) {
				down_state[hidk] = false;
				up_keys[up_key_count++] = hidk;
			}
		}
    }

    // we're only going to report on the first one, though we have to track down_state for all
	for (int i = 0; i < down_key_count; i++) {
		uint8_t hidcode = down_keys[i];

		printf("processing hidcode 0x%02x\n", hidcode);
		if (kbd_mode == Mode0_Compatibility) {
			uint16_t code = 0;

			if (ctrl) {
				printf(" + ctrl\n");
				code = s_code_table[hidcode][State_Control];
			} else if (shift) {
				printf(" + shift\n");
				code = s_code_table[hidcode][State_Shifted];
			} else {
				printf(" + unshifted\n");
				code = s_code_table[hidcode][State_Unshifted];
			}
			if (code != 0) {
				printf(" + translating hidcode 0x%02x to code 0x%04x and sending at %d baud\n", hidcode, code, BAUD_RATE);
				uart_putc_raw(UART_KBD_ID, code);
				printf(" + sent %04x\r\n", code);
			}
		} else {
			printf("uh...\r\n");
			// assume that Mode 1, 2, 3 all want key state
			// TODO
			// TODO mode1 will want us to translate modifier keys to up/down of that key as well
		}
	}
}

void apollo_dn300_mouse_report(hid_mouse_report_t const *report) {
	// big ol' TODO here.  from what I gather, mouse input is sent over the same
	// channel and is preceeded by a control character.
}

#define Yes 1
#define No 0
#define NOP 0

static char s_code_table[256][StateMax] = {
		                          /* Keycap          |Unshifted|Shifted|Control */
		                          /* Legend          |Code     |Code   |Code   */
		[HID_KEY_GRAVE]         = /* ~ '         */ { 0x60,     0x7E,   0x1E },
		[HID_KEY_ESCAPE]        = /* ESC         */ { 0x1B,     0x1B,   NOP  },
		[HID_KEY_1]             = /* ! 1         */ { 0x31,     0x21,   NOP  },
		[HID_KEY_2]             = /* @ 2         */ { 0x32,     0x40,   NOP  },
		[HID_KEY_3]             = /* # 3         */ { 0x33,     0x23,   NOP  },
		[HID_KEY_4]             = /* $ 4         */ { 0x34,     0x24,   NOP  },
		[HID_KEY_5]             = /* % 5         */ { 0x35,     0x25,   NOP  },
		[HID_KEY_6]             = /* ^ 6         */ { 0x36,     0x5E,   NOP  },
		[HID_KEY_7]             = /* & 7         */ { 0x37,     0x26,   NOP  },
		[HID_KEY_8]             = /* * 8         */ { 0x38,     0x2A,   NOP  },
		[HID_KEY_9]             = /* ( 9         */ { 0x39,     0x28,   NOP  },
		[HID_KEY_0]             = /* ) 0         */ { 0x30,     0x29,   NOP  },
		[HID_KEY_MINUS]         = /* _ -         */ { 0x2D,     0x5F,   NOP  },
		[HID_KEY_EQUAL]         = /* + =         */ { 0x3D,     0x2B,   NOP  },
		[HID_KEY_BACKSLASH]     = /* \ |         */ { 0x5C,     0x7C,   NOP  },
		[HID_KEY_BACKSPACE]     = /* BACKSPACE   */ { 0x08,     0x08,   NOP  },

		[HID_KEY_TAB]           = /* TAB         */ { 0xCA,     0xDA,   0xFA },
		[HID_KEY_Q]             = /* Q           */ { 0x71,     0x51,   0x11 },
		[HID_KEY_W]             = /* W           */ { 0x77,     0x57,   0x17 },
		[HID_KEY_E]             = /* E           */ { 0x65,     0x45,   0x05 },
		[HID_KEY_R]             = /* R           */ { 0x72,     0x52,   0x12 },
		[HID_KEY_T]             = /* T           */ { 0x74,     0x54,   0x14 },
		[HID_KEY_Y]             = /* Y           */ { 0x59,     0x59,   0x19 },
		[HID_KEY_U]             = /* U           */ { 0x75,     0x55,   0x15 },
		[HID_KEY_I]             = /* I           */ { 0x69,     0x49,   0x09 },
		[HID_KEY_O]             = /* O           */ { 0x6F,     0x4F,   0x0F },
		[HID_KEY_P]             = /* P           */ { 0x70,     0x50,   0x10 },
		[HID_KEY_BRACKET_LEFT]  = /* { [ / Ue    */ { 0x7B,     0x5B,   0x1B },
		[HID_KEY_BRACKET_RIGHT] = /* } ] / Oe    */ { 0x7D,     0x5D,   0x1D },
		                       // /* RETURN      */ { 0xCB,     0xDB,   NOP  },
		[HID_KEY_ENTER]         = /* RETURN      */ { 13,       13,     NOP  },

		[HID_KEY_A]             = /* A           */ { 0x61,     0x41,   0x01 },
		[HID_KEY_S]             = /* S           */ { 0x73,     0x53,   0x13 },
		[HID_KEY_D]             = /* D           */ { 0x64,     0x44,   0x04 },
		[HID_KEY_F]             = /* F           */ { 0x66,     0x46,   0x06 },
		[HID_KEY_G]             = /* G           */ { 0x67,     0x47,   0x07 },
		[HID_KEY_H]             = /* H           */ { 0x68,     0x48,   0x08 },
		[HID_KEY_J]             = /* J           */ { 0x6A,     0x4A,   0x0A },
		[HID_KEY_K]             = /* K           */ { 0x6B,     0x4B,   0x0B },
		[HID_KEY_L]             = /* L           */ { 0x6C,     0x4C,   0x0C },
		[HID_KEY_SEMICOLON]     = /* : ; / Oe    */ { 0x3B,     0x3A,   0xFB },
		[HID_KEY_APOSTROPHE]    = /* " ' / Ae    */ { 0x27,     0x22,   0xF8 },
// Apollo US keyboards have no hash key (#)
		//[HID_KEY_] = /* D14     ' #         */ { 0x23,     0x27,   NOP  },

		[HID_KEY_Z]             = /* Z           */ { 0x5A,     0x5A,   0x1A },
		[HID_KEY_X]             = /* X           */ { 0x78,     0x58,   0x18 },
		[HID_KEY_C]             = /* C           */ { 0x63,     0x43,   0x03 },
		[HID_KEY_V]             = /* V           */ { 0x76,     0x56,   0x16 },
		[HID_KEY_B]             = /* B           */ { 0x62,     0x42,   0x02 },
		[HID_KEY_N]             = /* N           */ { 0x6E,     0x4E,   0x0E },
		[HID_KEY_M]             = /* M           */ { 0x6D,     0x4D,   0x0D },
		[HID_KEY_COMMA]         = /* < ,         */ { 0x2C,     0x3C,   NOP  },
		[HID_KEY_PERIOD]        = /* > .         */ { 0x2E,     0x3E,   NOP  },
		[HID_KEY_SLASH]         = /* ? /         */ { 0xCC,     0xDC,   0xFC },
		[HID_KEY_SPACE]         = /* (space bar) */ { 0x20,     0x20,   0x20 },
		[HID_KEY_HOME]          = /* Home        */ { 0x84,     0x94,   0x84 },
#if !MAP_APOLLO_KEYS
		[HID_KEY_DELETE]        = /* DELETE      */ { 0x7F,     0x7F,   NOP  },
#else
		[HID_KEY_DELETE]        = /* POP         */ { 0x80,     0x90,   0x80 },
#endif  
		[HID_KEY_PAGE_UP]       = /* Roll Up     */ { 0x8D,     0x9D,   0x8D },
		[HID_KEY_PAGE_DOWN]     = /* Roll Down   */ { 0x8F,     0x9F,   0x8F },
		[HID_KEY_END]           = /* End         */ { 0x86,     0x96,   0x86 },
		[HID_KEY_ARROW_LEFT]    = /* Cursor left */ { 0x8A,     0x9A,   0x9A },
		[HID_KEY_ARROW_UP]      = /* Cursor Up   */ { 0x88,     0x98,   0x88 },
		[HID_KEY_ARROW_RIGHT]   = /* Cursor right*/ { 0x8C,     0x9C,   0x8C },
		[HID_KEY_ARROW_DOWN]    = /* Cursor down */ { 0x8E,     0x9E,   0x8E },

        // TODO mode1 will want us to translate modifier keys
#if false
		[HID_KEY_]              = /* CAPS LOCK   */ { NOP,      NOP,    NOP },
		[HID_KEY_]              = /* SHIFT       */ { NOP,      NOP,    NOP },
		[HID_KEY_]              = /* CTRL        */ { NOP,      NOP,    NOP },
// FIXME: ALT swapped!
		[HID_KEY_]              = /* ALT_R       */ { 0xfe00,   NOP,    NOP },
		[HID_KEY_]              = /* ALT_L       */ { 0xfe02,   NOP,    NOP },
		[HID_KEY_]              = /* SHIFT       */ { NOP,      NOP,    NOP },
#endif

#if false
#if !MAP_APOLLO_KEYS
		[HID_KEY_]              = /* Numpad CLR  */ { NOP,      NOP,    NOP  },
		[HID_KEY_]              = /* Numpad /    */ { NOP,      NOP,    NOP  },
		[HID_KEY_]              = /* Numpad *    */ { NOP,      NOP,    NOP  },
		[HID_KEY_]              = /* -           */ { 0xFE2D,   0xFE5F, NOP  },
		[HID_KEY_]              = /* 7           */ { 0xFE37,   0xFE26, NOP  },
		[HID_KEY_]              = /* 8           */ { 0xFE38,   0xFE2A, NOP  },
		[HID_KEY_]              = /* 9           */ { 0xFE39,   0xFE28, NOP  },
		[HID_KEY_]              = /* +           */ { 0xFE2B,   0xFE3D, NOP  },
		[HID_KEY_]              = /* 4           */ { 0xFE34,   0xFE24, NOP  },
		[HID_KEY_]              = /* 5           */ { 0xFE35,   0xFE25, NOP  },
		[HID_KEY_]              = /* 6           */ { 0xFE36,   0xFE5E, NOP  },
		[HID_KEY_]              = /* Numpad =    */ { NOP,      NOP,    NOP  },
		[HID_KEY_]              = /* 1           */ { 0xFE31,   0xFE21, NOP  },
		[HID_KEY_]              = /* 2           */ { 0xFE32,   0xFE40, NOP  },
		[HID_KEY_]              = /* 3           */ { 0xFE33,   0xFE23, NOP  },
		[HID_KEY_]              = /* ENTER       */ { 0xFECB,   0xFEDB, NOP  },
		[HID_KEY_]              = /* 0           */ { 0xFE30,   0xFE29, NOP  },
		[HID_KEY_]              = /* Numpad ,    */ { NOP,      NOP,    NOP  },
		[HID_KEY_]              = /* .           */ { 0xFE2E,   0xFE2E, NOP  },
		[HID_KEY_]              = /* F0          */ { 0x1C,     0x5C,   0x7C },
		[HID_KEY_]              = /* F1          */ { 0xC0,     0xD0,   0xF0 },
		[HID_KEY_]              = /* F2          */ { 0xC1,     0x01,   0xF1 },
		[HID_KEY_]              = /* F3          */ { 0xC2,     0x02,   0xF2 },
		[HID_KEY_]              = /* F4          */ { 0xC3,     0x03,   0xF3 },
		[HID_KEY_]              = /* F5          */ { 0xC4,     0x04,   0xF4 },
		[HID_KEY_]              = /* F6          */ { 0xC5,     0x05,   0xF5 },
		[HID_KEY_]              = /* F7          */ { 0xC6,     0x06,   0xF6 },
		[HID_KEY_]              = /* F8          */ { 0xC7,     0x07,   0xF7 },
		[HID_KEY_]              = /* F9          */ { 0x1F,     0x2F,   0x3F },
#else
		[HID_KEY_]              = /* POP         */ { 0x80,     0x90,   0x80 },
		[HID_KEY_]              = /* [<-]        */ { 0x87,     0x97,   0x87 },
		[HID_KEY_]              = /* [->]        */ { 0x89,     0x99,   0x89 },
		[HID_KEY_]              = /* Numpad -    */ { 0xFE2D,   0xFE5F, NOP  },
		[HID_KEY_]              = /* 7 Home      */ { 0x84,     0x94,   0x84 },
		[HID_KEY_]              = /* 8 Arrow Up  */ { 0x88,     0x98,   0x88 },
		[HID_KEY_]              = /* 9 Roll Up   */ { 0x8D,     0x9D,   0x8D },
		[HID_KEY_]              = /* Numpad +    */ { 0xFE2B,   0xFE3D, NOP  },
		[HID_KEY_]              = /* 4 Arrow L   */ { 0x8A,     0x9A,   0x9A },
		[HID_KEY_]              = /* NEXT WINDOW */ { 0x8B,     0x9B,   0x8B },
		[HID_KEY_]              = /* 6 Arrow R   */ { 0x8C,     0x9C,   0x8C },
		[HID_KEY_]              = /* Numpad =    */ { 0xFEC8,   0xFED8, NOP, },
		[HID_KEY_]              = /* 1 End       */ { 0x86,     0x96,   0x86 },
		[HID_KEY_]              = /* 2 Arrow Down*/ { 0x8E,     0x9E,   0x8E },
		[HID_KEY_]              = /* 3 Roll Down */ { 0x8F,     0x9F,   0x8F },
		[HID_KEY_]              = /* ENTER       */ { 0xFECB,   0xFEDB, NOP  },
		[HID_KEY_]              = /* NEXT WINDOW */ { 0x8B,     0x9B,   0x8B },
		[HID_KEY_]              = /* Numpad ,    */ { NOP,      NOP,    NOP  },
		[HID_KEY_]              = /* POP         */ { 0x80,     0x90,   0x80 },

		[HID_KEY_]              = /* F1/SHELL/CMD*/  { 0x85,    0x95,   0x85 },
		[HID_KEY_]              = /* F2/CUT/COPY */  { 0xB0,    0xB4,   0xB0 },
		[HID_KEY_]              = /* F3/UNDO/PASTE*/ { 0xB1,    0xB5,   0xB1 },
		[HID_KEY_]              = /* F4/MOVE/GROW*/  { 0xB2,    0xB6,   0xB2 },

		[HID_KEY_]              = /* F5/INS/MARK */ { 0x81,     0x91,   0x81 },
		[HID_KEY_]              = /* F6/LINE DEL */ { 0x82,     0x92,   0x82 },
		[HID_KEY_]              = /* F7/CHAR DEL */ { 0x83,     0x93,   0x83 },
		[HID_KEY_]              = /* F8/AGAIN    */ { 0xCD,     0xE9,   0xCD },

		[HID_KEY_]              = /* F9/READ      */ { 0xCE,    0xEA,   0xCE },
		[HID_KEY_]              = /* F10/SAVE/EDIT*/ { 0xCF,    0xEB,   0xCF },
#endif
		[HID_KEY_]              = /* F11/ABORT/EXIT*/ { 0xDD,   0xEC,   0xD0 },
		[HID_KEY_]              = /* F12/HELP/HOLD */ { 0xB3,   0xB7,   0xB3 },

		[HID_KEY_]              = /* NEXT WINDOW */ { 0x8B,     0x9B,   0x8B },
		[HID_KEY_]              = /* NEXT WINDOW */ { 0x8B,     0x9B,   0x8B },

// not yet used:
		[HID_KEY_]              = /* REPEAT        */ { NOP,    NOP,    NOP },
		[HID_KEY_]              = /* CAPS LOCK LED */ { NOP,    NOP,    NOP },
#endif
};
