              ;; Variant, change attribute value if you make your own
              .rtmodel cstartup,"c64-remote-debug"

              ;; External declarations
              .section stack
              .section cstack
              .section heap
              .section data_init_table

              .extern main, exit
              .extern _Vsp, _Zp

              .section programStart ; to be at address 0x801
              .word   nextLine
              .word   10            ; line number
              .byte   0x9e, " 2062", 0 ; SYS 2062
nextLine:     .word   0             ; end of program

              .section programStart
              .public __calypsi_entry, __program_start
__calypsi_entry:
__program_start:
              lda     #.byte0(.sectionEnd cstack)
              sta     zp:_Vsp
              lda     #.byte1(.sectionEnd cstack)
              sta     zp:_Vsp+1

;;; **** Initialize data sections if needed.
              .section programStart, noroot, noreorder
              .pubweak __data_initialization_needed
              .extern __initialize_sections
__data_initialization_needed:
              lda     #.byte0 (.sectionStart data_init_table)
              sta     zp:_Zp
              lda     #.byte1 (.sectionStart data_init_table)
              sta     zp:_Zp+1
              lda     #.byte0 (.sectionEnd data_init_table)
              sta     zp:_Zp+2
              lda     #.byte1 (.sectionEnd data_init_table)
              sta     zp:_Zp+3
              jsr     __initialize_sections

;;; **** Initialize streams if needed.
              .section programStart, noroot, noreorder
              .pubweak __call_initialize_global_streams
              .extern __initialize_global_streams
__call_initialize_global_streams:
              jsr     __initialize_global_streams

;;; **** Initialize heap if needed.
              .section programStart, noroot, noreorder
              .pubweak __call_heap_initialize
              .extern __heap_initialize, __default_heap
__call_heap_initialize:
              lda     #.byte0 __default_heap
              sta     zp:_Zp+0
              lda     #.byte1 __default_heap
              sta     zp:_Zp+1
              lda     #.byte0 (.sectionStart heap)
              sta     zp:_Zp+2
              lda     #.byte1 (.sectionStart heap)
              sta     zp:_Zp+3
              lda     #.byte0 (.sectionSize heap)
              sta     zp:_Zp+4
              lda     #.byte1 (.sectionSize heap)
              sta     zp:_Zp+5
              jsr     __heap_initialize

              .section programStart, root, noreorder
              tsx
              stx     _InitialStack ; for exit()
              lda     #0            ; argc = 0
              sta     zp:_Zp
              sta     zp:_Zp+1
              jsr     main
              jmp     exit

;;; ***************************************************************************
;;;
;;; Keep track of the initial stack pointer so that it can be restores to make
;;; a return back on exit().
;;;
;;; ***************************************************************************

              .section zdata, bss
              .public _InitialStack
_InitialStack:
              .space  1
