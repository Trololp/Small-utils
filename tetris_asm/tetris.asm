
; almost working normaly tetris

%define GRID_W 10
%define GRID_H 20
%define GRID_SZ 200

section .data
width_dq: dq 0
height_dq: dq 0

counter_dq: dq 0
counter2_dq: dq 0

figure_coord_x: dq 0
figure_coord_y: dq 0

grid: times 200 db 0

figures: db 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1 ; 6 figures
         db 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1


rotate_type: dq 0 ; 0 no rotate, 7 = rotate l, any other = rotate norm

have_figure: db 0 ; 0 no, 1 yes

figure_current: db 0, 0, 0, 0 ; buffer for rotating
                db 0, 0, 0, 0
                db 0, 0, 0, 0
                db 0, 0, 0, 0
                db 0, 0, 0, 0

rot_mask:
db 02h, 06h, 0Ah, 80h
db 01h, 05h, 09h, 80h
db 00h, 04h, 08h, 80h
db 80h, 80h, 80h, 80h


section .text
global game_init    ; Make the symbol visible to the linker
global game_run

extern width
extern height

extern pixels

extern key

; rax = figure
figure_to_tmp_buffer:
    push rdx
    push rbx
    push rcx

    mov [rel rotate_type], rax
    lea rdx, [rel figure_current]
    lea rbx, [rel figures]
    mov qword [rdx], 0
    mov qword [rdx + 8], 0



    cmp rax, 6
    jge .l_shape
    ; normals shape
    lea rcx, [rax + rax * 2]
    mov ecx, dword [rbx + rcx]
    and ecx, 0x00ffffff
    mov dword [rdx], ecx

    lea rcx, [rax + rax * 2]
    add rbx, 18
    mov ecx, dword [rbx + rcx]
    and ecx, 0x00ffffff
    mov dword [rdx + 4], ecx
    jmp .ret

    .l_shape:
    mov dword [rdx + 4], 0x01010101

    .ret:
    pop rcx
    pop rbx
    pop rdx
    ret

rotate_figure:
    push rax

    mov rax, [rel rotate_type]
    cmp rax, 7
    je .l_shape
    cmp rax, 0
    je .ret


    movdqu xmm0, [rel figure_current]
    movdqu xmm1, [rel rot_mask]
    pshufb xmm0, xmm1
    movdqu [rel figure_current], xmm0

    pop rax

    ret

    .l_shape:
    push rdx
    lea rdx, [rel figure_current]
    mov eax, dword [rdx]
    cmp eax, 0x00000100
    je .l_shape_2
    mov dword [rdx + 0],  0x00000100
    mov dword [rdx + 4],  0x00000100
    mov dword [rdx + 8],  0x00000100
    mov dword [rdx + 12], 0x00000100
    pop rdx
    jmp .ret

    .l_shape_2:
    mov dword [rdx + 0],  0
    mov dword [rdx + 4],  0x01010101
    mov dword [rdx + 8],  0
    mov dword [rdx + 12], 0
    pop rdx

    .ret:

    pop rax

    ret

    ; rax = x rbx = y
    ; rcx = result
check_coord:
    cmp rax, GRID_W
    jge .not_ok
    cmp rax, 0
    jl .not_ok
    cmp rbx, GRID_H
    jge .not_ok
    cmp rbx, 0
    jl .not_ok
    mov rcx, 1
    ret

    .not_ok:
    xor rcx, rcx
    ret

    ; rax = x rbx = y
tile_set:
    push rcx
    push rdx

    call check_coord
    test rcx, rcx
    jz .not_ok

    lea rcx, [rel grid]
    add rcx, rax
    mov rax, rbx
    mov rdx, GRID_W
    mul rdx
    add rcx, rax

    mov byte [rcx], 1

    .not_ok:
    pop rdx
    pop rcx
    ret

    ; cl = result
tile_check:
    push rdx

    call check_coord
    test rcx, rcx
    jz .not_ok

    lea rcx, [rel grid]
    add rcx, rax
    mov rax, rbx
    mov rdx, GRID_W
    mul rdx
    add rcx, rax

    mov cl, byte [rcx]
    pop rdx
    ret

    .not_ok:
    xor rcx,rcx
    pop rdx

    ret


tile_reset:
    push rcx
    push rdx

    call check_coord
    test rcx, rcx
    jz .not_ok

    lea rcx, [rel grid]
    xchg rbx, rax
    mov rdx, GRID_W
    mul rdx
    add rax, rbx

    mov byte [rcx + rax], 0

    .not_ok:
    pop rdx
    pop rcx
    ret

    ; rax = x rbx = y
tmp_buffer_to_tiles:
    ; iterate through tmp buff

    push rsi
    push rdx
    push rcx

    lea rsi, [rel figure_current]

    xor rdx, rdx

    .lp2:
    xor rcx, rcx

    .lp:
    cmp byte [rsi + rcx], 0
    je .skip
        push rax
        push rbx

        add rax, rcx
        sub rbx, rdx

        call tile_set

        pop rbx
        pop rax

    .skip:

    inc rcx
    cmp rcx, 4
    jl .lp
    inc rdx
    add rsi, 4
    cmp rdx, 4
    jl .lp2

    pop rcx
    pop rdx
    pop rsi

    ret

    ; rax = x rbx = y
tmp_buffer_clear_tiles:
    ; iterate through tmp buff

    push rsi
    push rdx
    push rcx

    lea rsi, [rel figure_current]

    xor rdx, rdx

    .lp2:
    xor rcx, rcx

    .lp:
    cmp byte [rsi + rcx], 0
    je .skip
        push rax
        push rbx

        add rax, rcx
        sub rbx, rdx

        call tile_reset

        pop rbx
        pop rax

    .skip:

    inc rcx
    cmp rcx, 4
    jl .lp
    inc rdx
    add rsi, 4
    cmp rdx, 4
    jl .lp2

    pop rcx
    pop rdx
    pop rsi

    ret

    ; rax = x, rbx = y
    ; rcx = direction
    ; 0 down, 1 left, 2 right
test_collision:
    test rcx, rcx
    jz .down_test
    mov cl, 0
    ret

    .down_test:

    push rsi
    push rdi
    push rdx


    xor rdi, rdi
    .lp2:
    xor rdx, rdx
    lea rsi, [rel figure_current]

    .lp1:
    cmp byte [rsi + rdi], 0
    je .skip
        .lp3:
        add rsi, 4
        inc rdx
        cmp byte [rsi + rdi], 0
        jne .lp3

        push rax
        push rbx

        add rax, rdi
        sub rbx, rdx

        cmp rbx, 0
        jl .hit2

        call tile_check
        test cl,cl
        jnz .hit2

        pop rbx
        pop rax

        jmp .tst

    .skip:
    add rsi, 4
    inc rdx
    .tst:
    cmp rdx, 4
    jl .lp1
    inc rdi
    cmp rdi, 4
    jl .lp2

    mov cl, 0
    jmp .ret

    .hit2:
    pop rbx
    pop rax

    mov cl, 1

    .ret:
    pop rdx
    pop rdi
    pop rsi

    ret


test_line:
    push rsi
    push rax
    push rbx

    jmp .test_lines

    .line:

    push rdi
    lea rdi, [rsi + GRID_W]

    .lp2_line:
    xor rax, rax
    .lp_line:

    mov cl, byte[rdi + rax]
    mov byte [rsi + rax], cl
    inc rax
    cmp rax, GRID_W
    jl .lp_line
    add rsi, GRID_W
    add rdi, GRID_W
    inc rbx
    cmp rbx, GRID_H - 1
    jl .lp2_line

    pop rdi

    .test_lines:

    lea rsi, [rel grid]
    xor rbx, rbx ; rbx y

    .lp2:

    xor rax, rax ; rax x

    .lp:

    cmp byte [rsi + rax], 0
    je .lp2_a
    inc rax
    cmp rax, GRID_W
    jl .lp
    jmp .line

    .lp2_a:
    add rsi, GRID_W
    inc rbx
    cmp rbx, GRID_H
    jl .lp2



    pop rbx
    pop rax
    pop rsi

    ret

game_init:
    push rcx
    push rax

    lea rcx, [rel height]
    xor rax, rax
    mov eax, dword [rcx]
    mov [rel height_dq], rax
    lea rcx, [rel width]
    mov eax, dword [rcx]
    mov [rel width_dq], rax


    pop rax
    pop rcx
    ret               ; Return

    ; rax = x, rbx = y ; edi = color
    ; tile 8x8 pixels
fill_tile:

    push rcx
    push rsi
    push rdx
    push rax
    push rbx

    shl rax, 5
    shl rbx, 5 ; multiply by 32

    mov rsi, 8
    mul qword [rel height_dq]
    mov rdx, rax
    lea rax, [rel pixels]
    mov rax, [rax]

    add rax, rdx
    add rax, rbx
    .lp2:
    mov rcx, 8


    .lp:
    mov dword [rax + rcx * 4], edi
    dec rcx
    jnz .lp
    mov rcx, [rel height_dq]
    shl rcx, 2
    add rax, rcx ; add height_dq * 4
    dec rsi
    jnz .lp2

    pop rbx
    pop rax
    pop rdx
    pop rsi
    pop rcx
    ret

grid_to_tiles:

    push rax
    push rbx
    push rdx

    lea rdx, [rel grid]
    xor rbx, rbx
    .lp2:
    xor rax, rax
    .lp:
    cmp byte [rdx + rax], 0
    je .a
        mov edi, 0xFF00FFFF
        call fill_tile
    .a:
    inc rax
    cmp rax, GRID_W
    jl .lp
    inc rbx
    add rdx, GRID_W
    cmp rbx, GRID_H
    jl .lp2

    pop rdx
    pop rbx
    pop rax
    ret

draw_border:
    push rax
    push rbx
    push rcx
    push rdi

    mov rax, GRID_W
    xor rbx, rbx

    mov edi, 0xFFA000A0

    .lp:
    call fill_tile
    inc rbx
    cmp rbx, GRID_H
    jle .lp

    .lp2:
    call fill_tile
    dec rax
    cmp rax, 0
    jge .lp2


    pop rdi
    pop rcx
    pop rbx
    pop rax

    ret

    ; rax = random value
get_random_v:
    push rdx
    rdtsc
    mov edx, 0x12345678
    mul rdx
    shr rax, 13
    and rax, 7
    pop rdx
    ret


handle_keys:
    push rcx
    push rax


    lea rcx, [rel key]
    mov eax, dword [rcx]
    xor rcx, rcx
    mov ecx, eax
    ; 8 up, 4 down, 2 left, 1 right

    cmp rcx, 8
    je .rotate
    cmp rcx, 4
    je .down

    test rcx, 3
    jz .exit

    shl rcx, 1
    ; 4 or 2
    sub rcx, 3
    ; 1 or -1

    mov rax, qword [rel figure_coord_x]
    sub rax, rcx
    mov qword [rel figure_coord_x], rax


    jmp .exit
    .down:
    mov rcx, qword [rel figure_coord_y]
    dec rcx
    mov qword [rel figure_coord_y], rcx
    jmp .exit

    .rotate:
    call rotate_figure

    .exit:
    pop rax
    pop rcx

    ret

    ; called every 1/60 sec
game_run:

    push rcx

    call draw_border

    call grid_to_tiles

    mov rcx, [rel counter_dq]
    inc rcx
    cmp rcx, 60
    jge .tick
    mov [rel counter_dq], rcx
    pop rcx
    ret



.tick:
    mov byte [rel counter_dq], 0
    mov cl, byte [rel have_figure]
    test cl, cl

    jnz .tick2
.new_figure:
    mov byte [rel have_figure], 1
    ; chose figure
    call get_random_v
    call figure_to_tmp_buffer

    ; draw moving figure
    mov qword [rel figure_coord_x], 4
    mov qword [rel figure_coord_y], 18 ; initial coords
.tick2:

    mov rax, qword [rel figure_coord_x]
    mov rbx, qword [rel figure_coord_y]
    call tmp_buffer_clear_tiles
    dec qword [rel figure_coord_y]

    call handle_keys

    ; check figure colliding
    mov rcx, 0
    mov rax, qword [rel figure_coord_x]
    mov rbx, qword [rel figure_coord_y]
    call test_collision
    test cl,cl
    jz .no_collision
    ; collided?

        ; handle wall collision

        ; handle fall collision
        ;
        mov rax, qword [rel figure_coord_x]
        mov rbx, qword [rel figure_coord_y]
        call tmp_buffer_to_tiles

        call test_line

        jmp .new_figure
            ; stack figure to pile

    .no_collision:




    ;timer
    ; get controlls
    ; figure move
    mov rax, qword [rel figure_coord_x]
    mov rbx, qword [rel figure_coord_y]
    call tmp_buffer_to_tiles


    pop rcx
    ret

