; theft cpu math instruction test
; This test is designed to run directly on the cpu, outside
; of any operating system or firmware context

; Pad the actual machine code up by one null-byte (0x00 [MACHINE CODE])
.nullpadding 1

.text
; First write 0xfeed to address 0x00ab on device 2
ld a, $#0002 ; Ready device number
st a, $6000  ; Set device number
ld a, $#feed ; Ready value to be written
st a, $00ab  ; Store the value to address 0x00ab

; Second try to load back the value in to register c
ld  c, $00ab   ; Load c from $00ab (High Byte of previously written data)
shl c, $#0008  ; Shift c left by 8 bits so that we can add on our low byte
add c, $00ac   ; Add low byte to c
shr c, $#0001  ; Shift the entire register right by one bit to test shr 

; Test the remaining math instructions
; Expected Result in a: 0x0bb4
ld  a, $#1337 ; Ready value to be operated on
sub a, $#0321 ; Subtract 0x0321 from a
and a, $#0b0c ; Bitwise and with mask 0x0b0c on a
or  a, $#0bb0 ; Bitwise or with mask 0x0bb0 on a
