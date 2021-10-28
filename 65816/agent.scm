;;; Debug agent is placed as address 0x8000 and up. This
;;; is the "reserved stack area" in the original kernel.
(define memories
  '((memory LoRAM (address (#x9600 . #x9fff))
            (section stack data zdata data heap))
    (memory LoCODE (address (#x8100 . #x95ff))
            (section code switch cdata data_init_table idata))
    (memory DirectPage (address (#x8000 . #x80ff))
            (section (registers ztiny)))
    (memory Vector (address (#xfff0 . #xffff))
            (section (reset #xfffc)))
    (block stack (size #x0600))
    (base-address _DirectPageStart DirectPage 0)
    ))
