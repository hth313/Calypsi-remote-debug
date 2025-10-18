(define memories
  '((memory RAM (address (#xd000 . #xdfff)) (type RAM)
      (section data stack zdata))
    (memory ROM (address (#xf000 . #xffe3)) (type ROM))
    (memory Vector (address (#xffe4 . #xffff)))
    ))
