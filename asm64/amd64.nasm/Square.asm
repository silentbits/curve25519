; 
; Copyright Mehdi Sotoodeh.  All rights reserved. 
; <mehdisotoodeh@gmail.com>
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that source code retains the 
; above copyright notice and following disclaimer.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
; "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
; LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
; A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
; OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
; LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;

%include "defines.inc"

; _______________________________________________________________________
;
;   void ecp_SqrReduce(U64* Y, const U64* X)
; _______________________________________________________________________
    PUBPROC ecp_SqrReduce

%define Y   ARG1
%define X   ARG2M
    PushB
    push    C1
    SaveArg2
    push    Y
    sub     rsp,64                  ; T[8]

%define T   rsp
%define U   rsp+32
                                    ; B3 | B2 | B1 | B0 | A3 | A2 | A1 | A0
    MULSET  A2,A1, [X],   [X+8]     ;    |         |         |  x0*x1  |
    MULSET  B0,A3, [X],   [X+24]    ;    |         |  x0*x3  |  x0*x1  |
    MULSET  B2,B1, [X+16],[X+24]    ;    |  x2*x3  |  x0*x3  |  x0*x1  |
    
                                    ;           C1 | C0 | B3 | A0
    MULSET  B3,A0, [X],  [X+16]     ;         |         |  x0*x2  |
    MULSET  C1,C0, [X+8],[X+24]     ;         |  x1*x3  |         |
    MULADD  C0,B3, [X+8],[X+16]     ; +  |         |  x1*x2  |
    adc     C1,0                    ; carry will be 0 here always

    ADD4    B1,B0,A3,A2, C1,C0,B3,A0
    adc     B2,0
    
    ; Multiply by 2
    shl     A1,1
    rcl     A2,1
    rcl     A3,1
    rcl     B0,1
    rcl     B1,1
    rcl     B2,1
    sbb     B3,B3
    neg     B3
    
    ; add diagonal values           ; y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0
    SQRSET  [T+8], [T],    [X]      ;         |         |         |  x0*x0
    SQRSET  [T+24],[T+16], [X+8]    ;         |         |  x1*x1  |
    SQRSET  [T+40],[T+32], [X+16]   ;         |  x2*x2  |         |
    SQRSET  [T+56],[T+48], [X+24]   ;  x3*x3  |         |         |
    
    ADD4    [T+32],[T+24],[T+16],[T+8], B0,A3,A2,A1
    adc     [T+40],B1
    adc     [T+48],B2
    adc     [T+56],B3

    ; Now do the size reduction: T[4] + 38*U[4]

    MULADD_W0 A0,[T],[U],38
    MULADD_W1 A1,[T+8],[U+8],38
    MULADD_W1 A2,[T+16],[U+16],38
    MULADD_W1 A3,[T+24],[U+24],38

    ; ZF set if ACH == 0
    jz      sqr_2
    MULT    38,ACH
    ADDA    0,0,ACH,ACL
    jnc     sqr_2
    
sqr_1:
    ADDA    0,0,0,38
    jc      sqr_1
    
sqr_2:
    add     rsp,64
    pop     ACH
    ; return result
    STOREA  ACH

    RestoreArg2
    pop     C1
    PopB
    ret