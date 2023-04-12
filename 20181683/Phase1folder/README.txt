System Project1

MyShell Phase1 

How to compile
    
    $make

How to run

    $./myshell

account

    1. show prompt
    2. read command
    3. parse command
    4. built-in command check
        (1)not built-in command -> fork
            -child process : execute command
            -parent process : wait child process
        (2)built-in command
            -cd 
            -exit
    

