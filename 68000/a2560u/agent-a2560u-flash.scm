(define memories
  '((memory vector (address (#xe00000 . #xe003ff))  (section reset))
    (memory loram  (address (#x00400 . #x013ff)) (type RAM) (section vectors6))
    (memory flash   (address (#xe00400 . #xe01fff)) (type ROM))
    ))
