;;
;; common functions for capone
;;

(define first car)
(define rest  cdr)

;; inc/dec familly
(define (inc n)
 (+ 1 n))

(define (dec n)
 (- n 1))

(define-macro (inc! n)
 `(set! ,n (+ 1 ,n)))

(define-macro (dec! n)
 `(set! ,n (- ,n 1)))

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
;; iota function; returns a list of numbers
;;
(define (iota n)
  (let loop ((n n)
			 (lst '()))
	(if (= n 0)
	  lst
	  (loop (- n 1) (cons n lst)))))

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
