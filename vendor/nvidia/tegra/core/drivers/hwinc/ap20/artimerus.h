//
// DO NOT EDIT - generated by simspec!
//

#ifndef ___ARTIMERUS_H_INC_
#define ___ARTIMERUS_H_INC_
//////////////////////////////////////////////////////////////////////////////////////////////////////
// Background information:
//
// The purpose of USEC_CFG/CNTR_1US registers is to provide a fix time base (in terms of microsecond) 
// to be use by the rest of the system regardless of what oscillator frequency coming in (i.e. 12MHz, 
// 13MHz, 19.2MHz, 26MHz, or even other frequencies). 
//
// USEC_CFG register:
// ==================
// Software should first configure this register by telling it what fraction of 1 micro-second each
// oscillator clock represent.  For example, if the osc clock is running at 12MHz, then each osc.
// clock represents 1/12 of a micro-second.
//   "USEC_DIVIDEND" and "USEC_DIVISOR" are used to indicate what fraction of 1 microsecond each osc 
//   clock represent.
//    osc clock freq.   dividend/divisor        USEC_DIVIDEND/USEC_DIVISOR
//    --------------------------------------------------------------------
//        12MHz                1/12                     0x00 / 0x0b
//        13MHz                1/13                     0x00 / 0x0c
//        19.2MHz              5/96                     0x04 / 0x5f
//        26MHz                1/26                     0x00 / 0x19
//
// CNTR_1US register:
// ==================
// This free-running read-only register/counter changes once every micro-second and is used mainly 
// by HW (and can as well be use by SW);  it starts counting from 0 once it's out of system reset
// and will continue counting forever unless oscillator clock is stopped or during a system reset.
//   (A) Software use:
//   -----------------
//        (1)  Although there's no interrupt mechanism for this register, SW can read the content of
//             this register multiple times to determine the amount of time that has been elapsed.
//   
//   (B)  Hardware use:
//   ------------------
//        (1)  It's used by the 4 timers (please see artimer.spec for more info.) to determine 
//             whether the programmed timer value has been reached; these 4 timers can trigger 
//             interrupts.
//        (2)  To provide a periodic usec pulse to be use by the flow controller to count the 
//             programmable number of micro-second before a flow control condition is triggered.
//        (3)  Use by secure boot logic.
//////////////////////////////////////////////////////////////////////////////////////////////////////

// Register TIMERUS_CNTR_1US_0  
#define TIMERUS_CNTR_1US_0                      _MK_ADDR_CONST(0x0)
#define TIMERUS_CNTR_1US_0_SECURE                       0x0
#define TIMERUS_CNTR_1US_0_WORD_COUNT                   0x1
#define TIMERUS_CNTR_1US_0_RESET_VAL                    _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_1US_0_RESET_MASK                   _MK_MASK_CONST(0xffffffff)
#define TIMERUS_CNTR_1US_0_SW_DEFAULT_VAL                       _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_1US_0_SW_DEFAULT_MASK                      _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_1US_0_READ_MASK                    _MK_MASK_CONST(0xffffffff)
#define TIMERUS_CNTR_1US_0_WRITE_MASK                   _MK_MASK_CONST(0x0)
// Elapsed time in micro-second 
#define TIMERUS_CNTR_1US_0_HIGH_VALUE_SHIFT                     _MK_SHIFT_CONST(16)
#define TIMERUS_CNTR_1US_0_HIGH_VALUE_FIELD                     (_MK_MASK_CONST(0xffff) << TIMERUS_CNTR_1US_0_HIGH_VALUE_SHIFT)
#define TIMERUS_CNTR_1US_0_HIGH_VALUE_RANGE                     31:16
#define TIMERUS_CNTR_1US_0_HIGH_VALUE_WOFFSET                   0x0
#define TIMERUS_CNTR_1US_0_HIGH_VALUE_DEFAULT                   _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_1US_0_HIGH_VALUE_DEFAULT_MASK                      _MK_MASK_CONST(0xffff)
#define TIMERUS_CNTR_1US_0_HIGH_VALUE_SW_DEFAULT                        _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_1US_0_HIGH_VALUE_SW_DEFAULT_MASK                   _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_1US_0_HIGH_VALUE_INIT_ENUM                 x

// Elapsed time in micro-second 
#define TIMERUS_CNTR_1US_0_LOW_VALUE_SHIFT                      _MK_SHIFT_CONST(0)
#define TIMERUS_CNTR_1US_0_LOW_VALUE_FIELD                      (_MK_MASK_CONST(0xffff) << TIMERUS_CNTR_1US_0_LOW_VALUE_SHIFT)
#define TIMERUS_CNTR_1US_0_LOW_VALUE_RANGE                      15:0
#define TIMERUS_CNTR_1US_0_LOW_VALUE_WOFFSET                    0x0
#define TIMERUS_CNTR_1US_0_LOW_VALUE_DEFAULT                    _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_1US_0_LOW_VALUE_DEFAULT_MASK                       _MK_MASK_CONST(0xffff)
#define TIMERUS_CNTR_1US_0_LOW_VALUE_SW_DEFAULT                 _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_1US_0_LOW_VALUE_SW_DEFAULT_MASK                    _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_1US_0_LOW_VALUE_INIT_ENUM                  x


// Register TIMERUS_USEC_CFG_0  
#define TIMERUS_USEC_CFG_0                      _MK_ADDR_CONST(0x4)
#define TIMERUS_USEC_CFG_0_SECURE                       0x0
#define TIMERUS_USEC_CFG_0_WORD_COUNT                   0x1
#define TIMERUS_USEC_CFG_0_RESET_VAL                    _MK_MASK_CONST(0xc)
#define TIMERUS_USEC_CFG_0_RESET_MASK                   _MK_MASK_CONST(0xffff)
#define TIMERUS_USEC_CFG_0_SW_DEFAULT_VAL                       _MK_MASK_CONST(0x0)
#define TIMERUS_USEC_CFG_0_SW_DEFAULT_MASK                      _MK_MASK_CONST(0x0)
#define TIMERUS_USEC_CFG_0_READ_MASK                    _MK_MASK_CONST(0xffff)
#define TIMERUS_USEC_CFG_0_WRITE_MASK                   _MK_MASK_CONST(0xffff)
// usec dividend.
#define TIMERUS_USEC_CFG_0_USEC_DIVIDEND_SHIFT                  _MK_SHIFT_CONST(8)
#define TIMERUS_USEC_CFG_0_USEC_DIVIDEND_FIELD                  (_MK_MASK_CONST(0xff) << TIMERUS_USEC_CFG_0_USEC_DIVIDEND_SHIFT)
#define TIMERUS_USEC_CFG_0_USEC_DIVIDEND_RANGE                  15:8
#define TIMERUS_USEC_CFG_0_USEC_DIVIDEND_WOFFSET                        0x0
#define TIMERUS_USEC_CFG_0_USEC_DIVIDEND_DEFAULT                        _MK_MASK_CONST(0x0)
#define TIMERUS_USEC_CFG_0_USEC_DIVIDEND_DEFAULT_MASK                   _MK_MASK_CONST(0xff)
#define TIMERUS_USEC_CFG_0_USEC_DIVIDEND_SW_DEFAULT                     _MK_MASK_CONST(0x0)
#define TIMERUS_USEC_CFG_0_USEC_DIVIDEND_SW_DEFAULT_MASK                        _MK_MASK_CONST(0x0)

// usec divisor.
#define TIMERUS_USEC_CFG_0_USEC_DIVISOR_SHIFT                   _MK_SHIFT_CONST(0)
#define TIMERUS_USEC_CFG_0_USEC_DIVISOR_FIELD                   (_MK_MASK_CONST(0xff) << TIMERUS_USEC_CFG_0_USEC_DIVISOR_SHIFT)
#define TIMERUS_USEC_CFG_0_USEC_DIVISOR_RANGE                   7:0
#define TIMERUS_USEC_CFG_0_USEC_DIVISOR_WOFFSET                 0x0
#define TIMERUS_USEC_CFG_0_USEC_DIVISOR_DEFAULT                 _MK_MASK_CONST(0xc)
#define TIMERUS_USEC_CFG_0_USEC_DIVISOR_DEFAULT_MASK                    _MK_MASK_CONST(0xff)
#define TIMERUS_USEC_CFG_0_USEC_DIVISOR_SW_DEFAULT                      _MK_MASK_CONST(0x0)
#define TIMERUS_USEC_CFG_0_USEC_DIVISOR_SW_DEFAULT_MASK                 _MK_MASK_CONST(0x0)


// Reserved address 8 [0x8] 

// Reserved address 12 [0xc] 

// Reserved address 16 [0x10] 

// Reserved address 20 [0x14] 

// Reserved address 24 [0x18] 

// Reserved address 28 [0x1c] 

// Reserved address 32 [0x20] 

// Reserved address 36 [0x24] 

// Reserved address 40 [0x28] 

// Reserved address 44 [0x2c] 

// Reserved address 48 [0x30] 

// Reserved address 52 [0x34] 

// Reserved address 56 [0x38] 

// Register TIMERUS_CNTR_FREEZE_0  
#define TIMERUS_CNTR_FREEZE_0                   _MK_ADDR_CONST(0x3c)
#define TIMERUS_CNTR_FREEZE_0_SECURE                    0x0
#define TIMERUS_CNTR_FREEZE_0_WORD_COUNT                        0x1
#define TIMERUS_CNTR_FREEZE_0_RESET_VAL                         _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_FREEZE_0_RESET_MASK                        _MK_MASK_CONST(0x13)
#define TIMERUS_CNTR_FREEZE_0_SW_DEFAULT_VAL                    _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_FREEZE_0_SW_DEFAULT_MASK                   _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_FREEZE_0_READ_MASK                         _MK_MASK_CONST(0x13)
#define TIMERUS_CNTR_FREEZE_0_WRITE_MASK                        _MK_MASK_CONST(0x13)
// 1 = freeze timers when COP is in debug state, 0 = no freeze.
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_COP_SHIFT                      _MK_SHIFT_CONST(4)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_COP_FIELD                      (_MK_MASK_CONST(0x1) << TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_COP_SHIFT)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_COP_RANGE                      4:4
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_COP_WOFFSET                    0x0
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_COP_DEFAULT                    _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_COP_DEFAULT_MASK                       _MK_MASK_CONST(0x1)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_COP_SW_DEFAULT                 _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_COP_SW_DEFAULT_MASK                    _MK_MASK_CONST(0x0)

// 1 = freeze timers when CPU1 is in debug state, 0 = no freeze.
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU1_SHIFT                     _MK_SHIFT_CONST(1)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU1_FIELD                     (_MK_MASK_CONST(0x1) << TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU1_SHIFT)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU1_RANGE                     1:1
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU1_WOFFSET                   0x0
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU1_DEFAULT                   _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU1_DEFAULT_MASK                      _MK_MASK_CONST(0x1)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU1_SW_DEFAULT                        _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU1_SW_DEFAULT_MASK                   _MK_MASK_CONST(0x0)

// 1 = freeze timers when CPU0 is in debug state, 0 = no freeze.
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU0_SHIFT                     _MK_SHIFT_CONST(0)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU0_FIELD                     (_MK_MASK_CONST(0x1) << TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU0_SHIFT)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU0_RANGE                     0:0
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU0_WOFFSET                   0x0
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU0_DEFAULT                   _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU0_DEFAULT_MASK                      _MK_MASK_CONST(0x1)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU0_SW_DEFAULT                        _MK_MASK_CONST(0x0)
#define TIMERUS_CNTR_FREEZE_0_DBG_FREEZE_CPU0_SW_DEFAULT_MASK                   _MK_MASK_CONST(0x0)


//
// REGISTER LIST
//
#define LIST_ARTIMERUS_REGS(_op_) \
_op_(TIMERUS_CNTR_1US_0) \
_op_(TIMERUS_USEC_CFG_0) \
_op_(TIMERUS_CNTR_FREEZE_0)


//
// ADDRESS SPACES
//

#define BASE_ADDRESS_TIMERUS    0x00000000

//
// ARTIMERUS REGISTER BANKS
//

#define TIMERUS0_FIRST_REG 0x0000 // TIMERUS_CNTR_1US_0
#define TIMERUS0_LAST_REG 0x0004 // TIMERUS_USEC_CFG_0
#define TIMERUS1_FIRST_REG 0x003c // TIMERUS_CNTR_FREEZE_0
#define TIMERUS1_LAST_REG 0x003c // TIMERUS_CNTR_FREEZE_0

#ifndef _MK_SHIFT_CONST
  #define _MK_SHIFT_CONST(_constant_) _constant_
#endif
#ifndef _MK_MASK_CONST
  #define _MK_MASK_CONST(_constant_) _constant_
#endif
#ifndef _MK_ENUM_CONST
  #define _MK_ENUM_CONST(_constant_) (_constant_ ## UL)
#endif
#ifndef _MK_ADDR_CONST
  #define _MK_ADDR_CONST(_constant_) _constant_
#endif

#endif // ifndef ___ARTIMERUS_H_INC_