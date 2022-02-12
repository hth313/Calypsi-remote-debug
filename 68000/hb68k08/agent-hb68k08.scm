(define memories
  '((memory monitor (address (#x8400 . #xafff))
      (placement-group bits
        (section reset ifar nearcode cfar switch data_init_table))
      (placement-group nobits
        (section zfar far vectors6 sstack)))
    (block sstack (size #x800))
    ))
