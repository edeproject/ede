(load "../lib/common.ss")

(define (map2 proc lst)
 (if (null? lst)
  lst
  (let loop ((lst lst)
			 (nls '()))
   (if (null? lst)
	(reverse nls)
	(loop (cdr lst) 
	      (cons (proc (car lst)) nls))))))

(define (map3 proc lst . more)
 (if (null? more)
  (map2 proc lst)
  (let map-more ((lst  lst)
	   			 (more more))
   (if (null? lst)
    lst
    (cons (apply proc (car lst) (map3 car more))
	      (map-more (cdr lst)
		            (map3 cdr more)))))))

(define v1 0)
(define v2 0)

(set! v1 (clock))
(define l (iota 1009))
;(define l (iota 10))
(set! v2 (clock))
(println "=== Pass 1: " (/ (- v2 v1) 1000000))

(println "Doing map...")

(set! v1 (clock))
(println (map3 (lambda (x) (+ 1 x)) l))
(set! v2 (clock))
(println "=== Pass 2: " (/ (- v2 v1) 1000000))
;(println (map + l l))

(println "Time is: " (timeit 
					  (lambda ()
					   (define v1 (iota 100))
					   (map + v1 v1)
					  )))

