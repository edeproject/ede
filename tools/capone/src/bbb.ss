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

(define lst (iota 9000))

(print "Working my map... ")
;; my map
(timeit-start)
(map3
 (fn (x)
  (+ 1 x)) lst) 
(timeit-end)
(println (timeit-result) " ms")

(print "Working with builtin map... ")
;; real map
(timeit-start)
(map
 (fn (x)
  (+ 1 x)) lst)
(timeit-end)
(println (timeit-result) " ms")

(print "Working my map [2]... ")
;; my map
(timeit-start)
(map3 + lst lst lst)
(timeit-end)
(println (timeit-result) " ms")

(print "Working with builtin map [2]... ")
;; real map
(timeit-start)
(map + lst lst lst)
(timeit-end)
(println (timeit-result) " ms")
