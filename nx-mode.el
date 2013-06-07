; Simple nx-mode designed as an extension of the tcl-mode in tcl.el
; in popular emacs distribitions
;
;                                        gustaf neumann (May 2013)
; TODO: 
; - complete definitions, 
; - maybe set indent level
; - finish var highlighting for nx variable syntax
;
; For now, load it e.g. with M-x load-library
; ~/.emacs.d/nx-mode.el

(load-library "tcl")

(setq nx-typeword-list (append tcl-typeword-list 
	  '("property")))

;; extra commands/methods to define something
(setq nx-proc-list (append tcl-proc-list 
	  '("method" "alias" "forward")))

;; Tcl control operators are rendered as keywords (if, while, ...
(setq nx-keyword-list tcl-keyword-list)

(setq nx-builtin-list (append tcl-builtin-list 
	  '("apply" "chan" "dict" 
	    "lassign" "lsearch" "lrepeat" "lreverse" "lset" 
	    "pkg_mkIndex" "refchan" "unload" "update"
	    "cget" "children" "configure" "create" "copy" 
	    "delete" "destroy" "filter" "has" "lookup" 
	    "heritage" "instances" "methods" "mixinof" 
	    "move" "object" "new" "parameter" "parent" 
	    "precedence" "require" "slots" "subclass" 
	    "superclass" "volatile" "variables" "vars" 
	    )))

(setq nx-typeword-regexp (regexp-opt nx-typeword-list 'words))
(setq nx-proc-regexp (regexp-opt nx-proc-list 'words))
(setq nx-keyword-regexp (regexp-opt nx-keyword-list 'words))
(setq nx-builtin-regexp (regexp-opt nx-builtin-list 'words))

(add-to-list 'myKeywords (cons nx-typeword-regexp 'font-lock-type-face))
(add-to-list 'myKeywords (cons nx-proc-regexp     'font-lock-function-name-face))
(add-to-list 'myKeywords (cons nx-keyword-regexp  'font-lock-keyword-face))
(add-to-list 'myKeywords (cons nx-builtin-regexp  'font-lock-builtin-face))


;;(message "My keywords is: %S" myKeywords)

(define-derived-mode nx-mode tcl-mode
  (setq font-lock-defaults '(myKeywords))
  (setq mode-name "NX Tcl")
)


;;;; the following section contains already an adapted regexp for vars in nx 
;;; (to highlight "set :x a" the same way as "set x a"), but this has still
;;; to be mangeled into the nicer style of above...

(setq myKeywords0
      (append 
       '("nx::Class\\|nx::Object\\|\\bmethod\\b\\|\\balias\\b\\|\\bforward\\b\\|\\bobject\\b\\|\\bproc\\b" . 'font-lock-function-name-face)
       '("\\bclass\\b\\|\\bcget\\b\\|\\bconfigure\\b\\|\\bcreate\\b\\|\\beval\\b\\|\\bfilter\\b\\|\\binfo\\b\\|\\blookup\\b\\|\\bmixin\\b\\|\\bsuperclass\\b" . 'font-lock-builtin-face)
;;       '(nx-typeword-regexp . 'font-lock-type-face)
       '("\\bproperty\\b\\|\\bprotected\\b\\|\\bprivate\\b\\|\\bpublic\\b\\|\\bvariable\\b\\|\\bupvar\\b" . 'font-lock-type-face)


       ;; (list (concat "\\(\\s-\\|^\\)"
       ;; 		 (regexp-opt nx-typeword-list t)
       ;; 		 "\\(\\s-\\|$\\)")
       ;; 	 2 'font-lock-type-face)
       
       ;; When variable names are enclosed in {} braces, any
       ;; character can be used. Otherwise just letters, digits,
       ;; underscores.  Variable names can be prefixed with any
       ;; number of "namespace::" qualifiers.  A leading "::" refers
       ;; to the global namespace.
       '("\\${\\([^}]+\\)}" 1 'font-lock-variable-name-face)
       '("\\$\\(\\(?:::\\)?\\(?:[[:alnum:]_]+::\\)*[[:alnum:]_]+\\)"  1 'font-lock-variable-name-face)
       '("\\(?:\\s-\\|^\\|\\[\\)set\\s-+{\\([^}]+\\)}" 1 'font-lock-variable-name-face 'keep)
       '("\\(?:\\s-\\|^\\|\\[\\)set\\s-+\\(\\(?:::\\)?\
\\(?:[[:alnum:]_]+::\\)*:?[[:alnum:]_]+\\)" 1 'font-lock-variable-name-face 'keep)
       )
      )