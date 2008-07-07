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
  "Check 'string<?' function"
  (begin
	(and
	  (string<? "aaaa" "z")
	  (string<? "foo" "fooo")
	  (not (string<? "foo" "asdadad"))
	  (not (string<? "fooooooooo" "ab"))
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
