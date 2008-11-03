;;
;; if-not expression
;;

(load "../lib/common.ss")

(ut-add-test
 "Check 'if-not' expression"
 (begin
  (define a 0)
  (define b 1)

  (and
   (if-not a #f #t)
   (if-not (= a 3) #t #f)
   (if-not (= b 1) #f #t)
   (if-not (= b 0) #t #f)
)))
