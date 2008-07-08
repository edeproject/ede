;;
;; string functions
;;

(ut-add-test
  "Check 'string?' function"
  (begin
	(and
	  (string? "sample string")
	  (not (string? 123))
	  (not (string? #\w))
	)))

(ut-add-test
  "Check 'string=?' function"
  (begin
	(and
	  (string=? "sample string" "sample string")
	  (not (string=? "sample string" "sample String"))
	  (not (string=? "ssss" ""))
	)))

(ut-add-test
  "Check 'string<? and string>?' functions"
  (begin
	(and
	  (string<? "aaaa" "z")
	  (string<? "foo" "fooo")
	  (not (string<? "foo" "asdadad"))
	  (not (string<? "fooooooooo" "ab"))

	  (string>? "z" "aaaa")
	  (string>? "fooo" "foo")
	  (string>? "foo" "asdadad")
	  (string>? "fooooooooo" "ab")
	)))

(ut-add-test
  "Check 'string<=? and string>=?' functions"
  (begin
    (and
	  (string<=? "aaaa" "z")
	  (string<=? "aaaa" "aaaa")
	  (string<=? "" "")
	  (not (string<=? "foo" "asdadad"))
	  (not (string<=? "fooooooooo" "ab"))

	  (string>=? "z" "aaaa")
	  (string>=? "aaaa" "aaaa")
	  (string>=? "" "")
	  (string>=? "foo" "asdadad")
	  (string>=? "fooooooooo" "ab")
	)))

(ut-add-test
  "Check 'string->list' function"
  (begin
    (let ((l (string->list "sample string")))
	  (and
	    (= 13 (length l))
		(char=? #\s (car l))
		(char=? #\a (cadr l))
		(char=? #\m (caddr l))
		(char=? #\p (cadddr l))
	))))

(ut-add-test
  "Check 'list->string' function"
  (begin
    (let ((s (list->string '(#\s #\a #\m #\p #\l #\e #\space #\s #\t #\r #\i #\n #\g))))
	  (string=? s "sample string")
	)))

(ut-add-test
  "Check 'string-length' function"
  (begin
    (and
	  (= 18 (string-length "some stupid sample"))
	  (= 0  (string-length ""))
	  (= 1  (string-length "a"))
	)))

;; As I could find from chicken and guile, string-fill! should modify
;; any string, not only one given with (make-string) only, which creates immutable strings
;;
;; This behaviour should be somehow documented and here it is:
;;  If we make a string like '(define s "foo")' it will be immutable by default and that
;;  can't be changed. On other hand, if we do something like:
;;   (define s (make-string 10))
;;   (set! s (string-copy "foo"))
;;  's' will have "foo" value, have length of 3 characters and _will_ be mutable, e.g.
;;  '(string-set! s 0 #\m)' will not fail and result will be "moo".
;;
;; Should it be seen as bug?
(ut-add-test
  "Check 'string-fill! and make-string' functions [!]"
  (begin
    (define s (make-string 11))
	(string-fill! s #\o)
	(string=? s "ooooooooooo")
	))

(ut-add-test
  "Check 'string-set!' function"
  (begin
    (define s (make-string 10))
	(set! s (string-copy "abrakadabra abrakadabra"))
	(string-set! s 0  #\A)
	(string-set! s 1  #\B)
	(string-set! s 2  #\A)
	(string-set! s 11 #\|)
	(string-set! s 22 #\A)
	(string=? s "ABAakadabra|abrakadabrA")
  )
)

(ut-add-test
  "Check 'number->string' function"
  (begin
    (and
	  (string=? "33"    (number->string 33))
	  (string=? "1234"  (number->string 1234))
	  (string=? "0"     (number->string 0))
	  (string=? "-1234" (number->string -1234))
	)))
