
(load "lib/common.ss")

(define *chars* 0)
(define *lines* 0)

(define fd (open-input-file "asciidoc.html"))
(set-input-port fd) ;; a bug in tinyscheme ?

(let loop [(a (read-char fd))]
  (if (eof-object? a)
	#f
	(begin
	  (set! *chars* (+ 1 *chars*))
	  (if (char=? a #\newline) (set! *lines* (+ 1 *lines*)))
	  (print *lines* "\r")
	  (loop (read-char fd)))))

(print "\nWe have " *chars* " characters and " *lines* " lines\n")
