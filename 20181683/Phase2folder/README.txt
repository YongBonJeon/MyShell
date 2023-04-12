system Project1 

MyShell Phase2
    
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
    5. pipe command check
        (1)pipe command -> fork
            -child process : execute process recursively
                output of process -> input of next proces
            -parent process : wait child process
        (2)not pipe command
            -same as 4-(1)

