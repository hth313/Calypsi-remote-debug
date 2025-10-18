(define memories
  '((memory loram (address (#x00000 . #x7ffff)) (fill 0)
      (section (reset #x00000) switch data_init_table ifar cfar nearcode))
    (memory workram  (address (#x80000 . #x8013ff)) (type RAM) (section vectors6))
    ))
