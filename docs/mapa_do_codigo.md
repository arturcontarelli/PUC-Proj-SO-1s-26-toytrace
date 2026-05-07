# Mapa do Código

**Onde o programa começa**: 

O programa começa no arquivo main.c, na função main.

**Onde o processo alvo é criado**: 

O processo alvo é criado na função launch_tracee dentro de trace_runtime.c, essa função cria o processo alvo e armazena o pid em child. 

**Onde o runtime chama o callback**: 

O callback trace_observer é passado como o ponteiro observer. A chamada real acontece no loop da função trace_program, localizada em trace_runtime.c, sendo executada como observer(&ev, userdata).

**Quais arquivos o grupo deve modificar**: 

O grupo deve modificar os arquivos trace_runtime.c, pairer.c e formatter.c. O trace_runtime tem TODO das semanas 2, 3 e 4, o pairer tem TODO da semana 2 e o formatter tem TODO da semana 5.

**Qual TODO aparece primeiro ao executar o scaffold**: 

O primeiro TODO que aparece está na função launch_tracee dentro de trace_runtime. A mensagem de erro é TODO Semana 2: implementar launch_tracee().

**Qual é a principal dúvida técnica do grupo neste momento**: 

A principal dúvida do grupo é entender como o Toytrace guarda as informações de uma syscall quando ela começa, como os argumentos, e depois usa essas informações junto com o valor de retorno quando a syscall termina.
