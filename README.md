# miniShell

Implementation (work in progress) of a basic Unix shell written in C as the first programming assignment for the Operational Systems course at the Federal University of São Paulo, under Prof. Bruno Kimura, Ph.D.

What works:
   * Simple commands, such as "ls", "ls -li", "clear", "help", etc;
   * Piped commands, such as "ls -li | tr s x". Commands with more than two pipes simultaneously than supported;
   * Saving to file, such as "ls -li > file_list";
   * Reading from file, such as "sort < file_list". This sometimes doesn't work, I'm not sure why, sadly.
  
What does not work:
   * Any mixture of special operators, such as "sort -n < origin > sorted";
   * Any sort of background support, such as the '&' operator or 'bg'.
   

Try it online [here.](https://minishell.jsbach.repl.run/)

Example:

![alt text](https://raw.githubusercontent.com/vrmiguel/miniShell/master/print.png?token=Aiqlgm5_CHEzLbQM1BbKi-CqnWMILWeFks5cuL1BwA%3D%3D)

By:
   Vinícius Rodrigues Miguel ([vrmiguel](https://github.com/vrmiguel)), Lucas S. Vaz ([lucasssvaz]https://github.com/vrmiguel) Gustavo Bárbaro de Oliveira ([GustavoBarbaro](https://github.com/GustavoBarbaro)).
