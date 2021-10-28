(define memories
  '((memory flash (address (#xa000 . #xafff))
            (section programStart code idata cdata switch data_init_table))
    (memory RAM (address (#xb000 . #xbfff))
            (section cstack data zdata heap))
    (memory ZPAGE (address (#x00 . #xff))
            (section (registers (#xca . #xff))))
    (memory STACK (address (#x100 . #x1ff))
            (section (stack (#x100 . #x1ff))))
    (block cstack (size #x800))
    (block stack (size #x100))
    (block heap (size #x200))
    ))