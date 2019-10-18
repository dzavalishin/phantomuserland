# sse3 + sse4 idea - NO, fail. BLENDP instr use imm blend map

 reg a = read pic z order movdqu


 pshufb a : all bytes == z

 reg b = read z buf

 pshuf b : 4 bytes (pixel) = z pos of corresp pizel

 sub a b > xmm0

; xmm0 = z diff, 4 eq bytes	per pixel

xmm1 = load screen pixels 
xmm2 = load bitmap pixels

 PBLENDVB xmm0 xmm1 xmm2 -> xmm1

xmm1 -> screen pixels



--------- OR ---------

load z buf pos as 32 bit integers, subtract as integers, 
convert upper byte to 4 same bytes per pixel (we need just sign bit)


VMOVDQU32 xmm1, z_map+shift ; load z orders for next 4 pixesl

MOVD xmm0, z_pos

; need extend lower 32 bits to 4 times 32 bits

PSHUFD xmm0, xmm0, 0x00 ; read all from lowest 32 bits

mov xmm6, xmm0 ; have copy of z_pos to update z buf later

;; NB can use direct PSHUFD xmm0, &z_pos, 0x00 - but z_pos must have min 128 bits.
;; use 2 instr (moved/pshufd) then.

; 4 pixels z order
PSUBD xmm0, xmm1

; now xmm0 has 4 differences of z orders in 4 top bits of 32 bit words

mov xmm7, xmm0 ; save diffs in word form in xmm7 - we'll need to update z orders later

pshufb a : all bytes == z


;; TODO update z buf here! we have xmm6 and 7 used now, and xmm1 has original z ordrs














