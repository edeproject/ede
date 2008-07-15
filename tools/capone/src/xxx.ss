
(load "../lib/common.ss")

;(define l (iota 1000))

(define i 0)

;(for i in l
; (print "The number is: " i "\n")
;)

;(while (< i 1000)
; (print "The number is: " i "\n")
; (set! i (+ i 1))
;)

;(print "Ret from dbus-send is: "
;	(dbus-send "SomeSignal" "org.equinoxproject.Demo" "/org/equinoxproject/Demo" "session" "foo")
;	"\n")

;(define m (dbus-proxy "SomeMethod" "org.equinoxproject.Demo" "/org/equinoxproject/Demo" "session"))
;(m "foo" "baz" "taz"):

(define l (re-split "[ \t_]+" "this_is\tsample string that should be tokenized    "))
(for i in l
 (print i "\n")
)

;(print (first (re-match "-" "some-sample-string" 0)) "\n")
;
;(define pos (re-match "http://(.*):" "http://www.google.com:8080"))
;(print pos "\n")
;(set! i (first pos))
;
;(while [< i (first (rest pos))]
; (print (string-ref "http://www.google.com:8080" i) "\n")
; (set! i (+ i 1))
;)
;;
;(set! l (re-split "</" "<a href=\"foo\">foo</a>xxx<a href=\"baz\">baz</a>"))
;(for i in l
; (print i "\n")
;)


;(print (re-replace "-" "@this--is-foo" "<p>") "\n")

;(println "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=")
;(println "          Capone System 0.1          ")
;(println "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=")
