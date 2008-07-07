;;
;; simple unittest code
;;

;; Here are stored functions that will be executed.
;; Each of them will be stored as list, function description and it's name
(define *registered-ut-code* '())

;; A functions for easier printing
(define (ut-print arg . rest)
  (display arg)
  (let loop ((rest rest))
	(if (not (null? rest))
	  (begin
		(display (car rest))
		(loop (cdr rest))))))

(define-macro (ut-println . body)
 `(ut-print ,@body "\n"))

;; Register a new function. Function should do some tests
;; and if they are correct it must return '#t' or '#f' if not
(define (ut-add-test-internal description func)
  (set! *registered-ut-code* (cons 
							   (list description func) 
							   *registered-ut-code*)))

;; A macro for easier usage of above function
(define-macro (ut-add-test descr . code)
 `(ut-add-test-internal ,descr
			   (lambda ()
				 ,(car code))))

;; Return how many there are tests
(define (ut-num-tests)
  (length *registered-ut-code*))

(define (compute-percent curr maximum)
  (/ (* 100 curr) maximum))

;; Calculate number of digits in given number
(define (num-digits n)
  (let loop ((n   n)
			 (ret 1))
	(if (and
		  (< n 10)
		  (> n -10))
	  ret
	  (loop (/ n 10) (+ ret 1)))))

;; Alling dots according to curr and maximum relationship
(define (print-dots curr maxnum)
  ;; let we start with at least 3 dots
  (ut-print "...")

  (let loop ([start (num-digits curr)]
			 [end   (num-digits maxnum)])
	(if (>= start end)
	  #t
	  (begin
		(ut-print ".")
		(loop (+ 1 start) end)))))

;; Run 'func' on each test. 'func' must have two parameters; first will
;; be functor and second will be it's description
(define (ut-run-all)
  (set! *registered-ut-code* (reverse *registered-ut-code*))
  (define i 1)
  (define ntests (ut-num-tests))
  
  (for-each
	(lambda (x)
	  (ut-print "[" i "/" ntests "]")

	  ;; print aligning dots
	  (print-dots i ntests)

	  (if ((cadr x))
		(ut-print "\033[32m[PASSED]\033[0m: ")
		(ut-print "\033[31m[FAILED]\033[0m: "))

	  ;; print description
	  (ut-println (car x))

	  (set! i (+ i 1)))
	*registered-ut-code*)
)
