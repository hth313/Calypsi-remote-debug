(define memories
  '((memory LoMem (address (#xff00 . #xffff)) (type ROM) (scatter-to HiFlash)
	    (section code cdata data_init_table idata
		     (reset #xfffc) (break #xffe6) (irq #xffee)))
    (memory Flash (address (#xffe000 . #xffffff)) (type ROM) (fill 0)
	    (section (HiFlash #xffff00) compactcode data_init_table idata switch))
    (memory LoRAM (address (#xf000 . #xfdff)) (type RAM))
    ))
