;;
;; common functions for capone
;;

(define first car)
(define rest  cdr)

;; inc/dec family
(define (inc n)
 (+ 1 n))

(define (dec n)
 (- n 1))

(define-macro (inc! n)
 `(set! ,n (+ 1 ,n)))

(define-macro (dec! n)
 `(set! ,n (- ,n 1)))

(define-macro (if-not . body)
 `(if (not ,(car body))
	 ,@(cdr body)))

(define-macro (var v val)
 `(define ,v ,val))

;;
;; Allow defining functions like:
;;  (def name (param1 param2)
;;    ...
;;  )
(define-macro (def name . rest)
 ;; name - function name
 ;; (car rest) - function params
 ;; (cdr rest)- function body
 `(define ,(cons name (car rest))
   ,@(cdr rest)))

;;
;; Flexible printing e.g.:
;;  (define num 3)
;;  (print "This number is: " num "\n")
;;
(define (print arg . rest)
  (display arg)
  (let loop ((rest rest))
	(if (not (null? rest))
	  (begin
		(display (car rest))
		(loop (cdr rest))))))

;;
;; (print) with newline
;;
(define-macro (println . body)
 `(print ,@body "\n"))

;;
;; while loop macro; used like:
;;  (while (> a 2)
;;   ...
;;  )
;;
(define-macro (while . body)
  `(let loop ()
	 ;; fetch condition
	 (if ,(car body)
	   (begin
		 ;; evaluate body
		 ,@(cdr body)
		 (loop)))))

;;
;; A python-like 'for' loop, works only on lists, like:
;; (for i in '(1 2 3 4 5)
;;  (print "Number is " i "\n")
;; )
(define-macro (for . expr)
 ;; (car expr) is 'i'
 ;; (caddr expr) is list
 ;; (cdddr expr) is body
 (let* (( lst (gensym) ))
   `(let (( ,lst ,(caddr expr) ))
	  (cond
		((list? ,lst)
		 (map (lambda (,(car expr))
				,@(cdddr expr)) 
			  ,lst))
		(else
		  (throw "Unsupported type in 'for' loop"))))))

;;
;; Split a list to a list of pairs so we can easily
;; embed it in 'let' expression via 'slet' macro
;; e.g. (1 2 3 4) => ((1 2) (3 4))
;;
(define (explode-list lst)
 (let loop ((lst lst)
			(n   '()))
  (if (null? lst)
   (reverse n)
   (begin
	;; huh...
	(set! n (cons (list (car lst) (cadr lst)) n))
	(loop (cddr lst) n)
))))

;;
;; slet or 'simplified let' is a 'let' with little less bracess
;; e.g. (let (a 1 b 2) body)
;;
(define-macro (slet . body)
 `(let ,@(list (explode-list (car body)))
	 ,@(cdr body)
))

(define-macro (slet* . body)
 `(let* ,@(list (explode-list (car body)))
	 ,@(cdr body)
))

;;
;; range function; returns a list of numbers in form [start end)
;;
;; Althought we could wrote this function cleanly without decrementors
;; using recursion call after 'cons', we would loose tail call optimization
;; yielding much slower function.
;;
(define (range start end)
  (let loop ((s (- start 1))
			 (e (- end   1))
			 (lst '()))
    (if (>= s e)
	  lst
	  (loop s (- e 1) (cons e lst)))))

;;
;; iota function; returns a list of numbers
;;
(define (iota n)
  (range 0 n))

;;
;; Inplace vector shuffle via Fisher-Yates algorithm
;;
(define (shuffle-vector! v)
 (let ((i (vector-length v))
	   (k 0)
	   (tmp 0))
  (while (> i 1)
   (set! k (modulo (random-next) i))
   (dec! i)
   (set! tmp (vector-ref v i))
   (vector-set! v i (vector-ref v k))
   (vector-set! v k tmp)
)))

;;
;; function for easier timing
;;
(define (timeit proc)
  (let ((v1 0)
		(v2 0))

    (set! v1 (clock))
	(proc)
    (set! v2 (clock))
    ;; 1000000 is value of CLOCKS_PER_SEC
	(/ (- v2 v1) 1000000)))

(define *timeit-start-value* 0)
(define *timeit-end-value* 0)

(define (timeit-start)
  (set! *timeit-start-value* (clock)))

(define (timeit-end)
  (set! *timeit-end-value* (clock)))

(define (timeit-result)
  (/ (- *timeit-end-value* *timeit-start-value*) 1000000))
