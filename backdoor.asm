;;   OS/2 Guest Tools for VMWare
;;   Copyright (C)2021, Rushfan
;;   Copyright (C)2023, Bankai Software bv
;;
;;   Licensed  under the  Apache License, Version  2.0 (the "License");
;;;  you may not use this  file  except in compliance with the License.
;;   You may obtain a copy of the License at                          
;; 
;;     http://www.apache.org/licenses/LICENSE-2.0
;;
;;   Unless  required  by  applicable  law  or agreed to  in  writing,
;;   software  distributed  under  the  License  is  distributed on an
;;   "AS IS"  BASIS, WITHOUT  WARRANTIES  OR  CONDITIONS OF ANY  KIND,
;;   either  express  or implied.  See  the  License for  the specific
;;   language governing permissions and limitations under the License.

	TITLE BackDoor API for VMWare
	PAGE 55,132
	.386
	
DGROUP  group   _DATA

_DATA   segment word public 'DATA'

;; MAGIC number to send to backdoor api
BDOOR_MAGIC	equ	564D5868h

;; Bit in reply (ECX) that indicates success
SUCCESS_BIT	equ	00010000h
DORECV_BIT      equ     00020000h

;; Low-bandwidth backdoor port number 
;; for the IN/OUT interface.
BDOOR_PORT	equ	5658h

_DATA   ends

assume  cs:_TEXT,ds:DGROUP

_TEXT   segment word public 'CODE'
        assume  CS:_TEXT


	
;; int Backdoor1(int cmd)
public Backdoor1_
Backdoor1_	proc	near
	push ebx
	push ecx
	push edx
 	mov ecx, eax
	mov eax, BDOOR_MAGIC
	mov ebx, 0
	mov dx, BDOOR_PORT
	in eax, dx
	pop edx
	pop ecx
	pop ebx
	ret
Backdoor1_	endp
	
;; int Backdoor2(int cmd, int func)
public Backdoor2_
Backdoor2_	proc	near
	push ebx
	push ecx
	push edx
 	mov ecx, eax
	mov ebx, edx
	mov eax, BDOOR_MAGIC
	mov dx, BDOOR_PORT
	in eax, dx
	pop edx
	pop ecx
	pop ebx
	ret
Backdoor2_	endp

;; Perform command, return all registers
;; void BackdoorAll(int cmd, int func, int[4] reg)
public BackdoorAll_
BackdoorAll_ proc near
	push ebx
	push ecx
	push edx
	push esi
	
	push ebx			; keep &reg
	mov ecx, eax			; cmd
	mov ebx, edx			; func

	mov eax, BDOOR_MAGIC
	mov dx, BDOOR_PORT
	in eax, dx
	
	pop esi
	mov [esi], eax
	mov [esi+4], ebx
	mov [esi+8], ecx
	mov [esi+12], edx
	
	pop esi
	pop edx
	pop ecx
	pop ebx
	ret
BackdoorAll_ endp


;; int BackdoorRPCOpen (int mode)	
public BackdoorRPCOpen_
BackdoorRPCOpen_ proc near
	push ebx
	push ecx
	push edx
	mov ebx, eax
	mov eax, BDOOR_MAGIC
	mov ecx, 0000001Eh		; CMD_MESSAGE | OPEN (== 0)
	mov dx, BDOOR_PORT
	in eax, dx
	test ecx, SUCCESS_BIT
	jnz open_success
	; FAIL
	mov eax, -1
	jmp open_end
open_success:		
	; EDX(hi) has the channel number
	mov eax, edx
	shr eax, 16
open_end:	
	pop edx
	pop ecx
	pop ebx
	ret
		
BackdoorRPCOpen_ endp

;; int BackdoorRPCSend (int channel, const char *out);
public BackdoorRPCSend_
BackdoorRPCSend_ proc near
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push es
	
	mov esi, edx    ; keep start of string
		
	; Determine length of string with scasb which uses ES:DI
	push ds
	pop es
	push eax	; ==channel number
	mov al, 0
	mov edi, esi
	mov ecx, 4096	; seems long enough
	repne scasb	; edi is now at 0 byte
	dec edi		; point to end of string 
	mov ebx, edi  
	sub ebx, esi    ; ebx now has length with 0 byte
	pop eax         ; channel nummber
	
	mov edx, BDOOR_PORT
	shl eax, 16
	or edx, eax     ; port + channel number
	mov eax, BDOOR_MAGIC
	mov ecx, 0001001Eh    		; CMD_MESSAGE | SEND_LENGTH
	in eax, dx
	test ecx, SUCCESS_BIT
	jnz send_send_data
	; FAIL
	mov eax, -1
	jmp send_end

	; At this point ESI is at the start of the string and EDI at the last
	; character (not the 0 byte). So we compare esi and edi to determine the 
	; end of the string. EDX contains the portnumber + channel, unchanged. 		
send_send_data:
	cmp esi, edi 
	jae send_done 			; all done
	lodsd				; get 4 bytes
	mov ebx, eax
	mov eax, BDOOR_MAGIC
	mov ecx, 0002001Eh		; CMD_MESSAGE | SEND_DATA
	in eax, dx
	test ecx, SUCCESS_BIT
	jnz send_send_data
	; FAIL
	mov eax, -2
	jmp send_end

send_done:
	mov eax, 0 
	; fall-through
	
send_end:	
	pop es
	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	ret
BackdoorRPCSend_ endp

;; int BackdoorRPCReceive (int channel, char *in, int len);
;; eax = channel, edx = *in, ebx = len
public BackdoorRPCReceive_
BackdoorRPCReceive_ proc near
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push es
		
	; setup stack, reserve 3 dwords 
	push ebp
	mov ebp, esp
	sub esp, 20
	mov dword ptr -4[ebp], 0	; return 
	mov -8[ebp], 0 			; reply_id
	shl eax, 16			; eax = 'channel', combine with port already
	or eax, BDOOR_PORT		
	mov -12[ebp], eax		; store for later use in edx
	mov -16[ebp], edx		; pointer to in
	mov -20[ebp], ebx		; len
	
	; Fetch length of reply by VM host
	mov edx, eax
	mov eax, BDOOR_MAGIC
	mov ecx, 0003001Eh		; CMD_MESSAGE | RECV_LENGTH
	in eax, dx			; if succesful ebx has the length
	test ecx, SUCCESS_BIT		; ecx should contain the the 'success' bit  +'dorecv' in case we have data
	jnz recv_got_length
	; FAIL
	mov dword ptr -4[ebp], -1
	jmp recv_end

recv_got_length:
	test ecx, DORECV_BIT		; No data? Then we're done
	jz recv_end
	
	shr edx, 16			; contains reply-ID
	mov -8[ebp], edx		
	cmp ebx, -20[ebp]		; check if we have enough buffer space
	jbe recv_got_2
	mov ebx, -20[ebp]		; nope: limit to buffer lenght ('len')

recv_got_2:
	mov -4[ebp], ebx		; Store expected number of bytes in return value
	push ds				; use EDI to store output in, but EDI operates on ES
	pop es
	mov edi, -16[ebp]		; get '*in' back
	mov esi, edi			; use ESI as end-of-buffer marker		
	add esi, ebx

recv_recv_loop:
	; Receive loop; compare edi/esi pointers, otherwise fetch data
	cmp edi, esi
	jae recv_done
	mov eax, BDOOR_MAGIC		
	mov ebx, -8[ebp]		; reply_id
	mov ecx, 0004001Eh		; CMD_MESSAGE | RECV_DATA
	mov edx, -12[ebp]
	in eax, dx			; hmm, edx(lo) gets reset
	test ecx, SUCCESS_BIT
	jz recv_fail
	mov eax, ebx
	stosd				; Store value, 4 bytes at a time
	jmp recv_recv_loop

recv_fail:	
	mov dword ptr -4[ebp], -2
	jmp recv_end
		
recv_done:	
	mov eax, BDOOR_MAGIC
	mov ebx, -8[ebp]		; final use of reply_id
	mov ecx, 0005001Eh		; CMD_MESSAGE | ACK_RECV
	mov edx, -12[ebp]
	in eax, dx
	test ecx, SUCCESS_BIT
	jnz recv_end
	mov dword ptr -4[ebp], -3
		
recv_end:	
	mov eax, -4[ebp]	; Retrieve return value
	mov esp, ebp		; Restore stack
	pop ebp
	pop es
	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	ret
BackdoorRPCReceive_ endp


;; void BackdoorRPCClose (int channel)
public BackdoorRPCClose_
BackdoorRPCClose_ proc near
	push eax
	push ebx
	push ecx
	push edx
	
	mov edx, BDOOR_PORT
	shl eax, 16
	or edx, eax 
	mov eax, BDOOR_MAGIC
	mov ecx, 0006001Eh		; CMD_MESSAGE | CLOSE
	in eax, dx
	
	pop edx
	pop ecx
	pop ebx
	pop eax
	ret
BackdoorRPCClose_ endp

_TEXT   ends
        end
       
