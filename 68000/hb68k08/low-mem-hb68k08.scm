(define memories
  '((memory vector (address (#x00000 . #x003ff))             (section reset))
    (memory fram   (address (#x00400 . #x07fff)) (type ROM))
    (memory sram   (address (#x08000 . #x0ffff)) (type RAM)  (section vectors6))
    ))
