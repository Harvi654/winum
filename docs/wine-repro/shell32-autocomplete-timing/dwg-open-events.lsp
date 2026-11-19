;; Original, AI-assisted (Codex), LGPL-2.1-or-later.
;; Transient reactors in the starting document's active namespace.

(defun winum-open-log (path message / stream)
  (if (setq stream (open path "a"))
    (progn
      (write-line message stream)
      (close stream)))
  (princ))

(defun winum-open-event (reactor arguments)
  (winum-open-log (vlr-data reactor)
                  (vl-princ-to-string (vlr-current-reaction-name))))

(defun winum-open-command (reactor arguments / name)
  (if (and arguments (eq (type (car arguments)) 'STR))
    (progn
      (setq name (vl-string-left-trim "._" (strcase (car arguments))))
      (cond
        ((= name "REGEN") (winum-open-log (vlr-data reactor) "control_regen"))
        ((= name "OPEN") (winum-open-log (vlr-data reactor) "command_open")))))
  (princ))

(defun winum-open-trace (path / reactor)
  (vl-load-com)
  (foreach reactor winum-open-reactors (vlr-remove reactor))
  (setq winum-open-reactors
    (list
      (vlr-command-reactor path '((:vlr-commandWillStart . winum-open-command)))
      (vlr-dwg-reactor path
        '((:vlr-beginDwgOpen . winum-open-event)
          (:vlr-dwgFileOpened . winum-open-event)
          (:vlr-endDwgOpen . winum-open-event)))
      (vlr-docmanager-reactor path
        '((:vlr-documentCreated . winum-open-event)
          (:vlr-documentToBeActivated . winum-open-event)
          (:vlr-documentBecameCurrent . winum-open-event)))))
  (winum-open-log path "reactors_ready"))

(defun winum-open-activex-document (log drawing / documents document)
  (setq documents (vla-get-documents (vlax-get-acad-object)))
  (winum-open-log log "activex_open_enter")
  (setq document (vla-open documents drawing :vlax-false))
  (winum-open-log log "activex_open_return")
  (if (= (strcase (vla-get-name document)) "PROBE.DWG")
    (winum-open-log log "activex_open_expected_dwg")))

(defun winum-open-activex (log drawing / result)
  (winum-open-log log "activex_begin")
  (setq result (vl-catch-all-apply 'winum-open-activex-document (list log drawing)))
  (if (vl-catch-all-error-p result)
    (winum-open-log log "activex_error"))
  (princ))

(defun winum-open-dbx-database (log drawing app database / active item lines)
  (setq active (vla-get-name (vla-get-activedocument app)))
  (winum-open-log log "dbx_open_enter")
  (vlax-invoke-method database 'Open drawing)
  (winum-open-log log "dbx_open_return")
  (setq lines 0)
  (vlax-for item (vla-get-modelspace database)
    (if (= (vla-get-objectname item) "AcDbLine")
      (setq lines (1+ lines))))
  (winum-open-log log (strcat "dbx_lines=" (itoa lines)))
  (winum-open-log log
    (if (= active (vla-get-name (vla-get-activedocument app)))
      "dbx_active_document=unchanged"
      "dbx_active_document=changed")))

(defun winum-open-dbx (log drawing / app database result)
  (winum-open-log log "dbx_begin")
  (setq app (vlax-get-acad-object))
  (setq database (vl-catch-all-apply 'vla-getinterfaceobject
                   (list app "ObjectDBX.AxDbDocument.26")))
  (if (vl-catch-all-error-p database)
    (winum-open-log log "dbx_object_unavailable")
    (progn
      (setq result (vl-catch-all-apply 'winum-open-dbx-database
                     (list log drawing app database)))
      (if (vl-catch-all-error-p result)
        (winum-open-log log "dbx_error"))
      (vlax-release-object database)))
  (princ))
