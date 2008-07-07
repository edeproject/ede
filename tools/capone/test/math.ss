;;
;; basic scheme math operators
;;

(ut-add-test
  "Check '+' operator"
  (begin
	(and
	  (= 3   (+ 1 2))
	  (= 1   (+ 1 0))
	  (= 120 (+ 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15))
	)))

(ut-add-test
  "Check '-' operator"
  (begin
	(and
	  (=  1   (- 3 2))
	  (= -1   (- 1 2))
	  (= -248 (- 100 99 98 97 12 13 14 15))
	)))

(ut-add-test
  "Check '*' operator"
  (begin
	(and
	  (= 0 (* 1 0))
	  (= 0 (* 0 1 2 3 4 5 6 7 8 9))
	  (= 362880 (* 1 2 3 4 5 6 7 8 9))
	)))

(ut-add-test
  "Check '/' operator"
  (begin
	(and
	  (= 2  (/ 4 2))
	  (= 40 (/ 80 2))
	  (= 2  (/ 1000 10 50))
	)))
