(define memories
  '((memory flash (address (#xe00000 . #xe7ffff)) (fill 0)
      (section (reset #xe00000) switch data_init_table ifar cfar nearcode))
    (memory loram  (address (#x00400 . #x013ff)) (type RAM) (section vectors6))
    ))
