; fix SDD 6.53 glitches on TNT+, run _before_ UNIVBE.EXE!
; --wbcbz7 25.o3.2o23

    org 0x100
start:
    ; dummy get VESA info call
    mov ax, 0x4f00
    mov di, vesabuf
    int 0x10

    ; dummy get mode info call
    mov ax, 0x4F01
    mov cx, 0x0112
    mov di, vesabuf
    int 0x10

    ; dummy 640x480 32bpp LFB mode set
    mov ax, 0x4F02
    mov bx, 0x4112
    int 0x10

    ; scroll to [0; 480]
    mov ax, 0x4f07
    xor bx, bx
    mov cx, 0
    mov dx, 480
    int 0x10

    ; and go back to textmode
    mov ax, 0x3
    int 0x10
    ret

vesabuf:
