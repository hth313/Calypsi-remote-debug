;;; Debug agent is placed as address 0x8000 and up. This
;;; is the "reserved stack area" in the original kernel.
(define memories
  '((memory LoRAM (address (#x8100 . #x9fff)) (type ANY))
    (memory Vector (address (#xfff0 . #xffff)))
    ))
