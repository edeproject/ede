(load "../lib/common.ss")

(define (map2 proc lst)
 (if (null? lst)
  lst
  (let loop ((lst lst)
			 (nls '()))
   (if (null? lst)
	nls
	(loop (cdr lst) (cons (proc (car lst)) nls))))))

(define l (iota 1009))
;(define l (iota 10))

(println "Doing map...")

(println 
 (map2 
  (lambda (x) (+ 1 x)) 
  l))
