;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; isr.S                                                   ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Módulo responsável pelos interrupts dos encoders, para  ;;
;; pegar o máximo de performance do hardware               ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PIND = 0x09
PINB = 0x03
SREG = 0x3F

.section .text

; ISR do grupo de pinos D
; leitura do encoder esquerdo
.global __vector_5
.extern lastD
.extern curFrameCountL
__vector_5:
  push r0
  in   r0, SREG        ; prólogo do ISR
  push r16
  ldi  r16, 0x0 
  
  lds  r1, lastD       ; r16 <- (last.1 ^ cur.0, last.0 ^ cur.1)
  sbic PIND, 2         ; se o primeiro canal estiver ativo
  ori  r16, 0x8        ; comuta o segundo bit do valor
  sbic PIND, 3         ; se o segundo canal estiver ativo
  ori  r16, 0x4        ; comuta o primeiro bit do valor
  eor  r16, r1

  cpi  r16, 0x8        ; se o resultado der 10, soma 
  breq .add

  cpi  r16, 0x4        ; se o resultado der 01, subtrai
  brne .final

.sub:
  lds  r16, curFrameCountL
  subi r16, 0x01
  sts  curFrameCountL, r16
  lds  r16, curFrameCountL+1
  sbci r16, 0x00
  sts  curFrameCountL+1, r16
  rjmp .final

.add:
  lds  r16, curFrameCountL
  subi r16, 0xFF
  sts  curFrameCountL, r16
  lds  r16, curFrameCountL+1
  sbci r16, 0xFF
  sts  curFrameCountL+1, r16

.final:
  in   r16, PIND
  andi r16, 0xc        ; guarda o novo valor do bit
  sts  lastD, r16

  eor  r1, r1
  pop  r16
  out  SREG, r0        ; epílogo do ISR
  pop  r0
  reti
