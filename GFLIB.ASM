; 
; gf_misc.asm - simple vga library
;

%      .MODEL memodel,C   ; command line support for selecting memory
                          ; model at compile time.
                          ; eg: ml /Dmemodel=compact /Cx /Cp /c
       .386

PUBLIC gf_setmode,gf_waitretrace
PUBLIC gf_setpalette,gf_get_palette_color,gf_set_palette_color
PUBLIC gf_setpalette_colors
PUBLIC gf_line,gf_pixel
PUBLIC gf_move_block,gf_fill_block

       .CODE

        ;****************************************;
        ;                                        ;
        ;  [ misc vga procedures ]               ;
        ;                                        ;
        ;****************************************;

gf_setmode PROC uses AX, mode:byte
       XOR    AX,AX
       MOV    AL,mode
       INT    10h
       RET
gf_setmode ENDP

;----------------------------------------------------------------------------

gf_waitretrace PROC uses AX DX
;
;   - waits for a vertical retrace start
;
       MOV    DX,3DAh
R1:    IN     AL,DX
       TEST   AL,8
       JNZ    R1
R2:    IN     AL,DX
       TEST   AL,8
       JZ     R2
       RET
gf_waitretrace ENDP


       ;****************************************;
       ;                                        ;
       ;  [ palette manipulation procedures ]   ;
       ;                                        ;
       ;****************************************;

gf_get_palette_color PROC uses DS SI AX DX,color:byte,r:far ptr,g:far ptr,b:far ptr
;
;   - reads a single DAC register
;
       ; tell VGA we're going to read a DAC register
       MOV    DX,3C6h
       MOV    AL,0FFh
       OUT    DX,AL

       ; tell VGA which register we are reading
       MOV    DX,3C7h
       MOV    AL,color     
       OUT    DX,AL        

       ADD    DX,2
       LDS    SI,r          ; data pointer
       IN     AL,DX         ; get red component
       MOV    [SI],AL
       LDS    SI,g
       IN     AL,DX         ; get green component
       MOV    [SI],AL
       LDS    SI,b
       IN     AL,DX         ; get blue component
       MOV    [SI],AL
       RET
gf_get_palette_color ENDP

gf_setpalette_colors PROC USES AX BX CX DX ES, palbuf:far ptr,start:word,num:word
;
;   - writes a group of DAC registers.
;
; As you can see, this uses a BIOS interrupt to write the palette, so you
; most probably do not want to use this in a tight loop, or in anything that is
; called more often than "every now and then". 
;
       LES    DX,palbuf                  ; ES:DX -> palette buffer
       MOV    AX,1012h                   ; Func: "set block of DAC regs"
       MOV    BX,start                   ; Index to start from
       MOV    CX,num                     ; Number of registers to write
       INT    10h                        ; Write the palette
       RET
gf_setpalette_colors ENDP
;----------------------------------------------------------------------------

gf_set_palette_color PROC uses AX DX, color:byte,r:byte,g:byte,b:byte
;
;  - writes a single DAC register
; 
       ; tell VGA we're going to write a DAC register
       ; I don't know if this is necessary, but here it is anyway
       MOV    DX,3C6h
       MOV    AL,0FFh
       OUT    DX,AL

       ; tell VGA which register we are writing
       MOV    DX,3C8h
       MOV    AL,color
       OUT    DX,AL

       ; write the values one at time
       INC    DX
       MOV    AL,r
       OUT    DX,AL
       MOV    al,g
       OUT    DX,AL
       MOV    AL,b
       OUT    DX,AL
       RET
gf_set_palette_color ENDP

;----------------------------------------------------------------------------

gf_setpalette PROC uses SI AX DX CX DS,palbuf:far ptr
;
;   - writes a group of DAC registers
;
       LDS    SI,palbuf      ; store address of palbuf -> ds:si
       MOV    DX,3C8h
       XOR    AL,AL          
       OUT    DX,AL          ; inform vga of palette change
       INC    DX
       MOV    CX,768         ; number of bytes to write (numregs * 3)
       CLD                    
       REP OUTSB              
       RET
gf_setpalette ENDP

;----------------------------------------------------------------------------

       ;******************************************
       ;
       ; graphic primitives (lines, pixels etc)
       ;
       ;******************************************

gf_pixel PROC uses ES DI BX AX,x:word,y:word,color:byte,buf:far ptr
;
; - plots a pixel to (x,y) of buf with color [color]
;
       LES    DI,buf                ; ES:DI -> destination buffer
       MOV    BX,x                  
       MOV    AX,y
       SHL    AX,8                  ; AX = 256 * y
       ADD    BX,AX                 ; BX = 256 * y + x
       SHR    AX,2                  ; AX =  64 * y
       ADD    BX,AX                 ; BX = 320 * y + x
       MOV    AL,color
       MOV    ES:[DI+BX],AL         ; write pixel
       RET
gf_pixel ENDP

;----------------------------------------------------------------------------

gf_line PROC uses ES DI AX BX CX DX,x:word,y:word,x2:word,y2:word,color:byte,buf:far ptr
;
;   - draws a line from (x,y) to (x2,y2).
;
; Not thought about optimizations yet, just directly hand assembled this
; from a C funtion.
; Wherever possible, I have included the C code in comments.
;
       MOV    AX,x
       SUB    x2,AX         ; x2 -= x;
       MOV    AX,y
       SUB    y2,AX         ; y2 -= y;
;
; Now, this conditional:
;
;      d1x = (x2 < 0) ? -1 : (x2 > 0) ? 1 : 0;
;
; after which we get the absolute value of x2 and store in BX for later use.
; CS:[d1x] is already 0, so we don't have to set it.
;
       MOV    AX,x2
       AND    AX,AX
       JZ     x2Done        ; zero: we're done and no need for ABS(x2) either
       CWD                  ; DX=-1 if AX is negative, DX=1 if AX is positive
       OR     DX,1          ; Result is either -1 or 1
       MOV    CS:[d1x],DX
;
; absolute value of x2 (in AX).
;
; TODO: Goddamn.. just zero the sign flag and you get the absolute value :)
;
       TEST   AH,80h        ; test sign bit (is AX negative?)
       JZ     x2Done        ; sign flag clear, this is the absolute value
       NEG    AX            ; two's complement negation to get absolute value
x2Done:
       MOV    BX,AX         ; copy value of x2 to BX for later use
       MOV    CS:[d2x],DX   ; d2x = d1x (DX);
       MOV    CS:[d2y],0    ; d2y = 0
;
; Now exactly the same thing as before, buf this time we set the value of
; d1y, which depends on the value of y2:
;
;      d1y = (y2 < 0) ? -1 : (y2 > 0) ? 1 : 0;
;
; CS:[d1y] is zero already
;
       MOV    AX,y2
       AND    AX,AX            
       JZ     y2Done
       CWD
       OR     DX,1
       MOV    CS:[d1y],DX
;
; get absolute value of y2 (AX) (n) and store it in DX.
;             
       TEST   AH,80h        
       JZ     y2Done        
       NEG    AX            
y2Done:
       MOV    DX,AX         ; DX = n
       MOV    AX,BX         ; AX = m
       CMP    AX,DX         ; m greater than n?
       JG     over          ; yes  
       MOV    CS:[d2x],0    ; Nope: { d2x = 0,
       MOV    BX,CS:[d1y]   
       MOV    CS:[d2y],BX   ;         d2y = d1y,
       XCHG   AX,DX         ;         swap n and m }
over:
       MOV    CS:[n],DX
       MOV    CS:[m],AX
       MOV    CX,AX
       INC    CX            ; CX = number of pixels to plot
       SHR    AX,1
       MOV    CS:[s],AX     ; s = m >> 1; 

       LES    DI,buf        ; store address of dest. buffer to ES:DI
       ;
       ; Start of plotting loop
       ;
lineloop:
; calculate pixel offset (320 * y + x)
       MOV    AX,y          ; AX = y
       MOV    BX,x          ; BX = x
       SHL    AX,8          ; AX = 256 * y
       ADD    BX,AX         ; BX = 256 * y + x
       SHR    AX,2          ; AX = 64 * y
       ADD    BX,AX         ; BX = 320 * y + x
                  
       MOV    AL,color      
       MOV    ES:[DI+BX],AL ; buf[320 * y + x] = color;  (plot a pixel)

       MOV    AX,CS:[n]
       ADD    CS:[s],AX     ; s += n;
       MOV    AX,CS:[s]
       CMP    AX,CS:[m]
       JL     LL1
; s >= m
       MOV    AX,CS:[m]
       SUB    CS:[s],AX     ; s -= m;
       MOV    AX,CS:[d1x]
       ADD    x,AX          ; x += d1x;
       MOV    AX,CS:[d1y]
       ADD    y,AX          ; y += d1y;
       JMP    LL2
LL1:
; s < m
       MOV    AX,d2x        
       ADD    x,AX          ; x += d2x;
       MOV    AX,d2y        
       ADD    y,AX          ; y += d2y;
LL2:
       LOOP   lineloop
       RET

d1x    DW     0
d1y    DW     0
d2x    DW     0
d2y    DW     0
m      DW     ?
n      DW     ?
s      DW     ?
gf_line ENDP

;----------------------------------------------------------------------------

gf_hline PROC uses ES DI AX BX,y:word,x:word,x2:word,color:byte,buf:far ptr
;
;   - draws a horizontal line from (x,y) to (x2,y)
;
       LES    DI,buf        ; load addres of buffer -> ES:DI

       MOV    BX,x          
       MOV    AX,y
       SHL    AX,8
       ADD    BX,AX
       SHR    AX,2
       ADD    BX,AX         ; BX = 320 * y + x
       ADD    DI,BX

       MOV    AL,color  
       MOV    AH,AL

       MOV    CX,x2
       SUB    CX,x          ; cx = length of line
       INC    CX
       SHR    CX,1          ; for stosw writes
       JNC    wplot
       STOSB                
wplot:
       REP STOSW
       RET
gf_hline ENDP


;============================================================================
;
; procedures for moving blocks of memory from one place to another and
; and filling memory blocks with values.
;
; not directly VGA related, but come handy when
; operating on double buffers and such.
;
; these procedures are fast due to operating with dwords, but therefore
; assume that nbytes % 4 == 0, where nbytes is size of the destination
; and/or source memory block.
; 

gf_move_block PROC uses SI DI DS ES CX,dest:far ptr,src:far ptr,nbytes:word
;
;    - copies nbytes bytes from src to dest.
;      assumes nbytes % 4 == 0
;
       LDS    SI,src      ; load ds:si with address of source block
       LES    DI,dest     ; load es:di with address of destination block
       MOV    CX,nbytes   ; cx = number of bytes to copy
       SHR    CX,2        ; divide by four to get number of dwords
       CLD
       REP MOVSD
       RET
gf_move_block ENDP

;----------------------------------------------------------------------------

gf_fill_block PROC uses EAX CX DI ES,block:far ptr,value:byte,nbytes:word
;
;    - fills a memory block with value.
;      assumes nbytes % 4 == 0
;
       LES    DI,block    ; load pointer to es:di
       MOV    CX,nbytes   ; get nbytes
       SHR    CX,2        ; nbytes = nbytes / 4 

       XOR    EAX,EAX
       MOV    AH,value    ;      0000 00??h
       MOV    AL,AH       ;      0000 ????h
       SHL    EAX,16      ;      ???? 0000h
       MOV    AH,value    ;      ???? 00??h
       MOV    AL,AH       ;      ???? ????h
       CLD
       REP STOSD
       RET
gf_fill_block ENDP

;============================================================================
;
;  Text blitter procedures
;
;============================================================================

        end
