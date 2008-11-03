;;
;; main unittest driver for capone
;;

;; Since 'utest.ss' is included here first, before
;; code that calls functions from it, the same code does
;; not need to include it again. 
;;
;; Otherwise, global counter in 'utest.ss' will be set to
;; empty list within each call and that is what we do not want
(load "utest.ss")
(load "math.ss")
(load "string.ss")
(load "if-not.ss")

(ut-println "")
(ut-println "  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=")
(ut-println "    Capone Test Suite From Hell  ")
(ut-println "      Ready to smack that CPU?   ")
(ut-println "  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=")
(ut-println "")

(ut-run-all)
