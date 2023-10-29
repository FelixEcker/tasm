; reserved addresses include file, to use include this file using the .inc
; directive within the .symbols section of your assembler source.

.symbols
; Applies only on ram device
STACK_PTR_STORE      = $0002

; Applies on every device when using st or ld
IO_DEVICE_STORE      = $6000
PROGRAM_BANK_STORE   = $6001
BRANCHING_MODE_STORE = $6002
RAM_DEVICE_STORE     = $6003
