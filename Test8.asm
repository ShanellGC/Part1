fun
    mov ax [bx]      
    cmp ax 30
    jb fail           
    cmp ax 50
    ja fail            
    ret ax            
fail:
    add bx 1         
    ret cx             
    
mov bx 100           
mov cx 0             
mov dx 0             

get dx
mov [bx] dx
call 0                
add cx ax

get dx
mov [bx+1] dx
call 0
add cx ax

get dx
mov [bx+2] dx
call 0
add cx ax

get dx
mov [bx+3] dx
call 0
add cx ax

get dx
mov [bx+4] dx
call 0
add cx ax

mov [200] cx         
mov ax cx            
put                    
halt

