;; Original, AI-assisted (Codex), LGPL-2.1-or-later.
;; Transient reactors in the starting document's active namespace.

(defun winum-open-log (path message / stream)
  (if (setq stream (open path "a"))
    (progn
      (write-line message stream)
      (close stream)))
  (if (and winum-trace-file-window
           (member message '("activex_open_enter" "activex_open_return" "activex_error"
                             "dbx_open_enter" "dbx_open_return" "dbx_error"
                             "save_command_enter" "save_begin" "save_command_exit"))
           (setq stream (open (strcat path "." message) "w")))
    (close stream))
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

(defun winum-save-command (reactor arguments / name)
  (setq name (car arguments))
  (if (and (eq (type name) 'STR)
           (= (vl-string-left-trim "._" (strcase name)) "SAVEAS"))
    (progn
      (winum-open-event reactor arguments)
      (if winum-trace-file-window
        (winum-open-log (vlr-data reactor)
          (if (eq (vlr-current-reaction-name) ':vlr-commandWillStart)
            "save_command_enter"
            "save_command_exit")))))
  (princ))

(defun winum-save-event (reactor arguments / data path)
  (setq data (vlr-data reactor)
        path (cadr arguments))
  (cond
    ((eq (vlr-current-reaction-name) ':vlr-beginSave)
     (winum-open-log (car data) "save_begin"))
    ((and (eq (type path) 'STR)
          (= (strcase (vl-string-translate "\\" "/" path)) (strcase (cadr data))))
     (winum-open-log (car data) "save_target_complete"))
    (T (winum-open-log (car data) "save_other_complete"))))

(defun winum-save-trace (log drawing)
  (winum-open-trace log)
  (setq winum-open-reactors
    (append
      (list
        (vlr-command-reactor log
          '((:vlr-commandWillStart . winum-save-command)
            (:vlr-commandEnded . winum-save-command)
            (:vlr-commandFailed . winum-save-command)
            (:vlr-commandCancelled . winum-save-command)))
        (vlr-dwg-reactor (list log drawing)
          '((:vlr-beginSave . winum-save-event)
            (:vlr-saveComplete . winum-save-event))))
      winum-open-reactors))
  (winum-open-log log "save_reactors_ready"))

(defun winum-save-activex-document (log drawing / document)
  (setq document (vla-get-activedocument (vlax-get-acad-object)))
  (winum-open-log log "activex_save_enter")
  (vla-saveas document (vl-string-translate "/" "\\" drawing) ac2018_dwg)
  (winum-open-log log "activex_save_return"))

(defun winum-save-activex (log drawing / result)
  (setq result (vl-catch-all-apply 'winum-save-activex-document (list log drawing)))
  (if (vl-catch-all-error-p result)
    (winum-open-log log "activex_save_error"))
  (princ))

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

(defun winum-new-document-activate (log app document role / marker)
  (setq marker (strcat "new_activate_" role))
  (winum-open-log log (strcat marker "_enter"))
  (vla-activate document)
  (winum-open-log log
    (strcat marker
      (if (= (vla-get-name document) (vla-get-name (vla-get-activedocument app)))
        "_active" "_other"))))

(defun winum-new-document-control (log / app documents original created count)
  (setq app (vlax-get-acad-object)
        documents (vla-get-documents app)
        original (vla-get-activedocument app)
        count (vla-get-count documents))
  (winum-open-log log "new_add_enter")
  (setq created (vla-add documents))
  (winum-open-log log "new_add_return")
  (winum-open-log log (strcat "new_count_delta=" (itoa (- (vla-get-count documents) count))))
  (winum-open-log log
    (if (= (vla-get-name created) (vla-get-name (vla-get-activedocument app)))
      "new_add_active=created" "new_add_active=other"))
  (winum-new-document-activate log app original "original")
  (winum-new-document-activate log app created "created"))

(defun winum-new-document (log / result)
  (winum-open-log log "new_begin")
  (setq result (vl-catch-all-apply 'winum-new-document-control (list log)))
  (if (vl-catch-all-error-p result)
    (winum-open-log log "new_error"))
  (princ))
