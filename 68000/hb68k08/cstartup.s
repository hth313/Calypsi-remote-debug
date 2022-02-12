;;; Startup variant, change attribute value if you make your own
              .rtmodel cstartup,"monitor"

              .rtmodel version, "1"

              ;; External declarations
              .section sstack
              .section heap
              .section data_init_table

              .extern main

#ifdef __CALYPSI_DATA_MODEL_SMALL__
              .extern _NearBaseAddress
#endif

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
              .public __program_root_section, __program_start
              .align  2
__program_root_section:
__program_start:
              move.l  #.sectionEnd sstack + 1,sp
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

              .section nearcode, root, noreorder
              moveq.l #0,d0         ; argc = 0
              bra.w   main
