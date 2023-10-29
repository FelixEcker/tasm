; initial orders program for theft cpu

.nullpadding 1

.symbols
.inc "reserved_addr.inc"
.test "a a aasdasdasd\nasdd mpapmpmp\taa\rb"
.test "asb"
.test "aa"
RAM_DEVICE : $02

.text
; Set primary ram device
ld a, RAM_DEVICE
st a, IO_DEVICE_STORE
st a, RAM_DEVICE_STORE

; Initialise CPU Stack
ld a, $#1000
st a, STACK_PTR_STORE
