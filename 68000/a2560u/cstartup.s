              .rtmodel cstartup,"foenix_monitor"

              .rtmodel version, "1"
              .rtmodel core, "*"

              ;; External declarations
              .section heap
              .section sstack
              .section data_init_table

              .extern main

#ifdef __CALYPSI_DATA_MODEL_SMALL__
              .extern _NearBaseAddress
#endif

#ifdef __CALYPSI_TARGET_SYSTEM_FOENIX__
              .extern _Gavin
# define GavinLow   0x000b0000
# define GavinHigh  0xfec00000
#endif

;;; ***************************************************************************
;;;
;;; The reset vector. This uses the entry point label __program_root_section
;;; which by default is what the linker will pull in first.
;;;
;;; ***************************************************************************

              .section reset
              .pubweak __program_root_section
__program_root_section:
              .long   .sectionEnd sstack + 1
              .long   __program_start

;;; ***************************************************************************
;;;
;;; __program_start - actual start point of the program
;;;
;;; Initialize sections and call main().
;;; You can override this with your own routine, or tailor it as needed.
;;; The easiest way to make custom initialization is to provide your own
;;; __low_level_init which gets called after stacks have been initialized.
;;;
;;; ***************************************************************************

              .section nearcode, noreorder
              .pubweak __program_start
              .align  2
__program_start:
#ifdef __CALYPSI_DATA_MODEL_SMALL__
              lea.l   _NearBaseAddress.l,a4
#endif

;;; Initialize data sections if needed.
              .section nearcode, noroot, noreorder
              .pubweak __data_initialization_needed
              .extern __initialize_sections
              .align  2
__data_initialization_needed:
              move.l  #(.sectionStart data_init_table),a0
              move.l  #(.sectionEnd data_init_table),a1
              bsr.w   __initialize_sections

;;; **** Initialize streams if needed.
              .section nearcode, noroot, noreorder
              .pubweak __call_initialize_global_streams
              .extern __initialize_global_streams
__call_initialize_global_streams:
              bsr.w   __initialize_global_streams

;;; **** Initialize heap if needed.
              .section nearcode, noroot, noreorder
              .pubweak __call_heap_initialize
              .extern __heap_initialize, __default_heap
__call_heap_initialize:
              move.l  #.sectionSize heap,d0
              move.l  #__default_heap,a0
              move.l  #.sectionStart heap,a1
              bsr.w   __heap_initialize

              .section nearcode, noroot, noreorder
#ifdef __CALYPSI_TARGET_SYSTEM_FOENIX__
              .pubweak _Gavin_initialize
_Gavin_initialize:
              move.l  #GavinLow,a0  ; assume A2560U system
              cmp.w   #0x4567,0x0010(a0) ; check byte order
              beq.s   20$
              move.l  #GavinHigh,a0 ; no, assume A2560K 32-bit
20$:
              ;; keep base pointer to Gavin
#ifdef __CALYPSI_DATA_MODEL_SMALL__
              move.l  a0,(.near _Gavin,A4)
#else
              move.l  a0,_Gavin
#endif // __CALYPSI_DATA_MODEL_SMALL__
#endif // __CALYPSI_TARGET_SYSTEM_FOENIX__

              .section nearcode, root, noreorder
              moveq.l #0,d0         ; argc = 0
              bra.w   main
