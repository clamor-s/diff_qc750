//
// DO NOT EDIT - generated by simspec!
//

#ifndef ___ARTIMER_H_INC_
#define ___ARTIMER_H_INC_
// The system has four 29-bit timers that run at a 1-MHz clock rate based on the crystal
// oscillator. Each timer can be programmed to generate single, periodic, or watchdog interrupts.
// When the EN bit is set high, the timer loads the timer present trigger value (PTV) into its
// counter and starts decrementing at a 1-MHz clock rate (1us increments). 
// When the timer present count value (PCR) decrements to zero, it generates a timer interrupt.
// When the enable periodic interrupt (PER) bit is enabled, the timer generates an interrupt 
// and reloads the counter with the PTV value and starts to decrement again. 
//
// Note: If the xtal oscillator or clock source is disabled during low-power standby mode, the
// microsecond timers will not function. This also means that interrupts will not occur.
//Before programming timer:
//Input oscillator frequency must be specified in the TIMERUS_USEC_CFG to support multiple frequencies
//Program the number of 1us timer counts per "tick" in the PTV (Present Trigger Value) register. 
//Set the PTV EN (enable bit) to start counting down.
//When the count reaches zero, interrupt is generated.
//To auto-reload the timer counter, set the PTV PER (periodic) bit. Otherwise, leave it clear for a one-shot.
// Timer Present Trigger Value (Set) Register

// Register TIMER_TMR_PTV_0  
#define TIMER_TMR_PTV_0                 _MK_ADDR_CONST(0x0)
#define TIMER_TMR_PTV_0_WORD_COUNT                      0x1
#define TIMER_TMR_PTV_0_RESET_VAL                       _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_RESET_MASK                      _MK_MASK_CONST(0xffffffff)
#define TIMER_TMR_PTV_0_SW_DEFAULT_VAL                  _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_SW_DEFAULT_MASK                         _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_READ_MASK                       _MK_MASK_CONST(0xffffffff)
#define TIMER_TMR_PTV_0_WRITE_MASK                      _MK_MASK_CONST(0xffffffff)
// Enable Timer 
#define TIMER_TMR_PTV_0_EN_SHIFT                        _MK_SHIFT_CONST(31)
#define TIMER_TMR_PTV_0_EN_FIELD                        (_MK_MASK_CONST(0x1) << TIMER_TMR_PTV_0_EN_SHIFT)
#define TIMER_TMR_PTV_0_EN_RANGE                        31:31
#define TIMER_TMR_PTV_0_EN_WOFFSET                      0x0
#define TIMER_TMR_PTV_0_EN_DEFAULT                      _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_EN_DEFAULT_MASK                 _MK_MASK_CONST(0x1)
#define TIMER_TMR_PTV_0_EN_SW_DEFAULT                   _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_EN_SW_DEFAULT_MASK                      _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_EN_DISABLE                      _MK_ENUM_CONST(0)
#define TIMER_TMR_PTV_0_EN_ENABLE                       _MK_ENUM_CONST(1)

// Enable Periodic Interrupt
#define TIMER_TMR_PTV_0_PER_SHIFT                       _MK_SHIFT_CONST(30)
#define TIMER_TMR_PTV_0_PER_FIELD                       (_MK_MASK_CONST(0x1) << TIMER_TMR_PTV_0_PER_SHIFT)
#define TIMER_TMR_PTV_0_PER_RANGE                       30:30
#define TIMER_TMR_PTV_0_PER_WOFFSET                     0x0
#define TIMER_TMR_PTV_0_PER_DEFAULT                     _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_PER_DEFAULT_MASK                        _MK_MASK_CONST(0x1)
#define TIMER_TMR_PTV_0_PER_SW_DEFAULT                  _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_PER_SW_DEFAULT_MASK                     _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_PER_DISABLE                     _MK_ENUM_CONST(0)
#define TIMER_TMR_PTV_0_PER_ENABLE                      _MK_ENUM_CONST(1)

// Reserved 
#define TIMER_TMR_PTV_0_N_A_SHIFT                       _MK_SHIFT_CONST(29)
#define TIMER_TMR_PTV_0_N_A_FIELD                       (_MK_MASK_CONST(0x1) << TIMER_TMR_PTV_0_N_A_SHIFT)
#define TIMER_TMR_PTV_0_N_A_RANGE                       29:29
#define TIMER_TMR_PTV_0_N_A_WOFFSET                     0x0
#define TIMER_TMR_PTV_0_N_A_DEFAULT                     _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_N_A_DEFAULT_MASK                        _MK_MASK_CONST(0x1)
#define TIMER_TMR_PTV_0_N_A_SW_DEFAULT                  _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_N_A_SW_DEFAULT_MASK                     _MK_MASK_CONST(0x0)

// Trigger Value: count trigger value (count length).  This
// is in n+1 scheme.  If you program the value n, the count
// trigger value will actually be n+1.
#define TIMER_TMR_PTV_0_TMR_PTV_SHIFT                   _MK_SHIFT_CONST(0)
#define TIMER_TMR_PTV_0_TMR_PTV_FIELD                   (_MK_MASK_CONST(0x1fffffff) << TIMER_TMR_PTV_0_TMR_PTV_SHIFT)
#define TIMER_TMR_PTV_0_TMR_PTV_RANGE                   28:0
#define TIMER_TMR_PTV_0_TMR_PTV_WOFFSET                 0x0
#define TIMER_TMR_PTV_0_TMR_PTV_DEFAULT                 _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_TMR_PTV_DEFAULT_MASK                    _MK_MASK_CONST(0x1fffffff)
#define TIMER_TMR_PTV_0_TMR_PTV_SW_DEFAULT                      _MK_MASK_CONST(0x0)
#define TIMER_TMR_PTV_0_TMR_PTV_SW_DEFAULT_MASK                 _MK_MASK_CONST(0x0)

//When interrupt is generated, write "1" to PCR[30] present count Regsiter) to clear the interrupt 
//Timer interrupt assignments: 
//Timer 1 = primary interrupt controller bit 0 
//Timer 2 = primary interrupt controller bit 1 
//Timer 3 = secondary interrupt controller bit 9 
//Timer 4 = secondary interrupt controller bit 10 
// Timer Present Count Value (Status)Register

// Register TIMER_TMR_PCR_0  
#define TIMER_TMR_PCR_0                 _MK_ADDR_CONST(0x4)
#define TIMER_TMR_PCR_0_WORD_COUNT                      0x1
#define TIMER_TMR_PCR_0_RESET_VAL                       _MK_MASK_CONST(0x0)
#define TIMER_TMR_PCR_0_RESET_MASK                      _MK_MASK_CONST(0xffffffff)
#define TIMER_TMR_PCR_0_SW_DEFAULT_VAL                  _MK_MASK_CONST(0x0)
#define TIMER_TMR_PCR_0_SW_DEFAULT_MASK                         _MK_MASK_CONST(0x0)
#define TIMER_TMR_PCR_0_READ_MASK                       _MK_MASK_CONST(0xffffffff)
#define TIMER_TMR_PCR_0_WRITE_MASK                      _MK_MASK_CONST(0x40000000)
// Reserved = 0
#define TIMER_TMR_PCR_0_N_A2_SHIFT                      _MK_SHIFT_CONST(31)
#define TIMER_TMR_PCR_0_N_A2_FIELD                      (_MK_MASK_CONST(0x1) << TIMER_TMR_PCR_0_N_A2_SHIFT)
#define TIMER_TMR_PCR_0_N_A2_RANGE                      31:31
#define TIMER_TMR_PCR_0_N_A2_WOFFSET                    0x0
#define TIMER_TMR_PCR_0_N_A2_DEFAULT                    _MK_MASK_CONST(0x0)
#define TIMER_TMR_PCR_0_N_A2_DEFAULT_MASK                       _MK_MASK_CONST(0x1)
#define TIMER_TMR_PCR_0_N_A2_SW_DEFAULT                 _MK_MASK_CONST(0x0)
#define TIMER_TMR_PCR_0_N_A2_SW_DEFAULT_MASK                    _MK_MASK_CONST(0x0)

// 1 = clears the interrupt, 0 = no affect. Wtite-1-to-Clear
#define TIMER_TMR_PCR_0_INTR_CLR_SHIFT                  _MK_SHIFT_CONST(30)
#define TIMER_TMR_PCR_0_INTR_CLR_FIELD                  (_MK_MASK_CONST(0x1) << TIMER_TMR_PCR_0_INTR_CLR_SHIFT)
#define TIMER_TMR_PCR_0_INTR_CLR_RANGE                  30:30
#define TIMER_TMR_PCR_0_INTR_CLR_WOFFSET                        0x0
#define TIMER_TMR_PCR_0_INTR_CLR_DEFAULT                        _MK_MASK_CONST(0x0)
#define TIMER_TMR_PCR_0_INTR_CLR_DEFAULT_MASK                   _MK_MASK_CONST(0x1)
#define TIMER_TMR_PCR_0_INTR_CLR_SW_DEFAULT                     _MK_MASK_CONST(0x0)
#define TIMER_TMR_PCR_0_INTR_CLR_SW_DEFAULT_MASK                        _MK_MASK_CONST(0x0)

// Reserved = 0 
#define TIMER_TMR_PCR_0_N_A1_SHIFT                      _MK_SHIFT_CONST(29)
#define TIMER_TMR_PCR_0_N_A1_FIELD                      (_MK_MASK_CONST(0x1) << TIMER_TMR_PCR_0_N_A1_SHIFT)
#define TIMER_TMR_PCR_0_N_A1_RANGE                      29:29
#define TIMER_TMR_PCR_0_N_A1_WOFFSET                    0x0
#define TIMER_TMR_PCR_0_N_A1_DEFAULT                    _MK_MASK_CONST(0x0)
#define TIMER_TMR_PCR_0_N_A1_DEFAULT_MASK                       _MK_MASK_CONST(0x1)
#define TIMER_TMR_PCR_0_N_A1_SW_DEFAULT                 _MK_MASK_CONST(0x0)
#define TIMER_TMR_PCR_0_N_A1_SW_DEFAULT_MASK                    _MK_MASK_CONST(0x0)

// Counter value: decrements from PTV.
#define TIMER_TMR_PCR_0_TMR_PCV_SHIFT                   _MK_SHIFT_CONST(0)
#define TIMER_TMR_PCR_0_TMR_PCV_FIELD                   (_MK_MASK_CONST(0x1fffffff) << TIMER_TMR_PCR_0_TMR_PCV_SHIFT)
#define TIMER_TMR_PCR_0_TMR_PCV_RANGE                   28:0
#define TIMER_TMR_PCR_0_TMR_PCV_WOFFSET                 0x0
#define TIMER_TMR_PCR_0_TMR_PCV_DEFAULT                 _MK_MASK_CONST(0x0)
#define TIMER_TMR_PCR_0_TMR_PCV_DEFAULT_MASK                    _MK_MASK_CONST(0x1fffffff)
#define TIMER_TMR_PCR_0_TMR_PCV_SW_DEFAULT                      _MK_MASK_CONST(0x0)
#define TIMER_TMR_PCR_0_TMR_PCV_SW_DEFAULT_MASK                 _MK_MASK_CONST(0x0)


//
// REGISTER LIST
//
#define LIST_ARTIMER_REGS(_op_) \
_op_(TIMER_TMR_PTV_0) \
_op_(TIMER_TMR_PCR_0)


//
// ADDRESS SPACES
//

#define BASE_ADDRESS_TIMER      0x00000000

//
// ARTIMER REGISTER BANKS
//

#define TIMER0_FIRST_REG 0x0000 // TIMER_TMR_PTV_0
#define TIMER0_LAST_REG 0x0004 // TIMER_TMR_PCR_0

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

#endif // ifndef ___ARTIMER_H_INC_