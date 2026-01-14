  CPU 8086
  ORG 0x100
dest EQU 0x500
start:
  mov di, dest
  mov si, payload
  push di
  ; SI = src
  ; DI = dst
  ; AX = 0
  ; BX = 0
  ; CH = 0
  dec bx
.copy_literals:
  call .get_elias
  rep movsb
  jc .new_offset
.copy_offset:
  call .get_elias
%if ZPACK_EXT_CPY
  inc cx
%endif
  push si
  lea si, [di + bx]
  rep movsb
  pop si
  jnc .copy_literals
.new_offset:
  xchg ax, bx
  lodsb
  xchg ax, bx
  inc bx
  jnz .copy_offset
.ret:
  shl al, 1
  ret
.get_elias:
  mov cl, 1
.read_elias:
  shl al, 1
  jnz .skip
  lodsb
  stc
  adc al, al
.skip:
  jnc .ret
  shl al, 1
  adc cx, cx
  jmp .read_elias
payload:
