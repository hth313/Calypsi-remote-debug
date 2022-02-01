(define memories
  '((memory far-bits (address (#x60000 . #x9ffff))
            (section (mystart #x60000) reset ifar nearcode
                     libcode cfar switch data_init_table))
    (memory near-area (address (#xa0000 . #xbffff))
            (placement-group near-bits (section cnear near))
            (placement-group near-nobits (section znear)))
    (memory far-nobits (address (#xc0000 . #xd7fff))
            (section far zfar heap stack))
    (memory SupervisorStack (address (#x8000 . #xffff))
            (section sstack))
    (block sstack (size #x800))
    (block stack (size #x800))
    ))
